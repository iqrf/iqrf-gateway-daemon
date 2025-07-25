/*
* filename: IqrfTcp.h
* author: Karel Hanák <xhanak34@stud.fit.vutbr.cz>
* school: Brno University of Technology, Faculty of Information Technology
* bachelor's thesis: Automatic Testing of Software
*
* Header file for the IqrfTcp component.
*
* Copyright 2020 Karel Hanák
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

#pragma once

#include "IIqrfChannelService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <string>

namespace iqrf {
  class ITraceService;

  class IqrfTcp : public IIqrfChannelService {
  public:
    class Imp;

    IqrfTcp();
    virtual ~IqrfTcp();

    void startListen() override;
    State getState() const override;
    std::unique_ptr<Accessor> getAccess(ReceiveFromFunc receiveFromFunc, AccesType access) override;
    bool hasExclusiveAccess() const override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    Imp* m_imp = nullptr;
  };
}
