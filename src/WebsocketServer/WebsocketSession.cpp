/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "WebsocketSession.h"
#include "Trace.h"

#include <iostream>

#define SESSION_LOG(session, addr, port) "[" << session << "|" << addr << ":" << port << "] "

WebsocketSession::WebsocketSession(std::size_t id, boost::asio::ip::tcp::socket&& socket, boost::asio::ssl::context& ctx): m_id(id), m_stream(std::move(socket), ctx) {
  auto endpoint = m_stream.next_layer().lowest_layer().remote_endpoint();
  m_address = endpoint.address().to_string();
  m_port = endpoint.port();
}

std::size_t WebsocketSession::getId() const {
  return m_id;
}

const std::string& WebsocketSession::getAddress() const {
  return m_address;
}

uint16_t WebsocketSession::getPort() const {
  return m_port;
}

void WebsocketSession::send(const std::string& message) {
  boost::asio::post(
    m_stream.get_executor(),
    [self = shared_from_this(), message]() mutable {
      self->m_writeQueue.push_back(std::move(message));
      if (!self->m_writing) {
        self->write();
      }
    }
  );
}

void WebsocketSession::write() {
  m_writing = true;
  m_stream.async_write(
    boost::asio::buffer(m_writeQueue.front()),
    boost::beast::bind_front_handler(
      &WebsocketSession::on_write,
      shared_from_this()
    )
  );
}

void WebsocketSession::on_write(boost::beast::error_code ec, std::size_t bytesWritten) {
  if (ec) {
    TRC_WARNING(
      SESSION_LOG(m_id, m_address, m_port)
      << "Failed to write message to stream: "
      << BEAST_ERR_LOG(ec)
    );
  } else {
    m_writeQueue.pop_front();
    if (!m_writeQueue.empty()) {
      this->write();
    } else {
      m_writing = false;
    }
  }
}

void WebsocketSession::run() {
  boost::asio::dispatch(
    m_stream.get_executor(),
    boost::beast::bind_front_handler(
      &WebsocketSession::on_run,
      shared_from_this()
    )
  );
}

void WebsocketSession::on_run() {

  // IF TLS DO THIS, ELSE CALL ACCEPT
  boost::beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(30));

  m_stream.next_layer().async_handshake(
    boost::asio::ssl::stream_base::server,
    boost::beast::bind_front_handler(
      &WebsocketSession::on_handshake,
      shared_from_this()
    )
  );
}

void WebsocketSession::on_handshake(boost::beast::error_code ec) {
  if (ec) {
    TRC_WARNING(
      SESSION_LOG(m_id, m_address, m_port)
      << "Failed to complete handshake: "
      << BEAST_ERR_LOG(ec)
    );
    return;
  }
  this->accept();
}

void WebsocketSession::accept() {
  boost::beast::get_lowest_layer(m_stream).expires_never();

  m_stream.set_option(
    boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server)
  );

  m_stream.set_option(
    boost::beast::websocket::stream_base::decorator(
      [](boost::beast::websocket::response_type& res) {
        res.set(
          boost::beast::http::field::server,
          "iqrf-gateway-daemon (" + std::string(BOOST_BEAST_VERSION_STRING) + ")"
        );
      }
    )
  );

  m_stream.async_accept(
    boost::beast::bind_front_handler(
      &WebsocketSession::on_accept,
      shared_from_this()
    )
  );
}

void WebsocketSession::on_accept(boost::beast::error_code ec) {
  if (ec) {
    TRC_WARNING(
      SESSION_LOG(m_id, m_address, m_port)
      << "Failed to accept client connection: "
      << BEAST_ERR_LOG(ec)
    );
  } else {
    if (this->onOpen) {
      this->onOpen(m_id, ec);
    }
    this->read();
  }
}

void WebsocketSession::read() {
  m_stream.async_read(
    m_buffer,
    boost::beast::bind_front_handler(
      &WebsocketSession::on_read,
      shared_from_this()
    )
  );
}

void WebsocketSession::on_read(boost::beast::error_code ec, std::size_t bytesRead) {
  boost::ignore_unused(bytesRead);

  if (ec == boost::beast::websocket::error::closed ||
      ec == boost::asio::error::eof ||
      ec.category() == boost::asio::ssl::error::stream_category
  ) {
    TRC_INFORMATION(
      SESSION_LOG(m_id, m_address, m_port)
      << "Connection closed: "
      << BEAST_ERR_LOG(ec)
    );
    if (this->onClose) {
      this->onClose(m_id, ec);
    }
    return;
  }

  if (ec) {
    TRC_WARNING(
      SESSION_LOG(m_id, m_address, m_port)
      << "Failed to read incoming message from stream: "
      << BEAST_ERR_LOG(ec)
    );
    if (this->onClose) {
      this->onClose(m_id, ec);
    }
    return;
  }

  if (this->onMessage) {
    std::string message = boost::beast::buffers_to_string(m_buffer.data());
    this->onMessage(m_id, message);
  }

  m_buffer.consume(m_buffer.size());
  this->read();
}

void WebsocketSession::close() {
  boost::asio::post(
    m_stream.get_executor(),
    [self = shared_from_this()]() {
      self->m_stream.async_close(
        boost::beast::websocket::close_code::normal,
        [self](boost::beast::error_code ec) {
          if (ec) {
            TRC_WARNING(
              SESSION_LOG(self->m_id, self->m_address, self->m_port)
                << "Failed to close WebSocket session gracefully: "
                << BEAST_ERR_LOG(ec)
            );
          } else {
            TRC_INFORMATION(
              SESSION_LOG(self->m_id, self->m_address, self->m_port)
                << "WebSocket session closed gracefully."
            );
          }
        }
      );
    }
  );
}
