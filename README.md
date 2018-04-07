# Structure

- **Documets:** Anything related to official documentation (md files, images, uml, ...)
- **Ideas:** Any non official stuff  (docs, images, files, e-mails, etc...)
- **Trash:** To be deleted stuff.

# Requirements

- Product specification [product_specification.md](Documents/01-Requirements/product-specification.md)
- API requirements [api-requirements.md](Documents/01-Requirements/api-requirements.md)
- Software Requirements [sw-requirements.md](Documents/01-Requirements/sw-requirements.md)

# Design documents

- software design document [api-design.md](Documents/02-Design/sw-design.md)

# Tools and Guidelines

- Document flow [README.md](Documents/03-Tools&Guidelines/README.md)

# Project status

- We use **GitLab Board** for driving API definition project: https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/boards

#Win developement instalation

Install:
- [shape](https://github.com/logimic/shape) (launcher, logging, ...)
- [shapeware](https://github.com/logimic/shapeware) (cpprest, websockets, ...)
- `./build64.bat` or `./build64.bat`
- Copy libs according #66 (temporary up to appropriate deployment)
- install paho: vcpkg install paho-mqtt
