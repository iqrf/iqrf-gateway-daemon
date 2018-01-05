# Software design [#18](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/18)

## 1 Terms definition

**Module** is distributable software in form of shared library. Includes components data classes and ligically merges different SW parts.

**Service** provides data services like send data, parse data, calculate data, log data and more.

**Interface** is pure virtual (abstract) class containing methods (functions). Used parameters should be as much general as possible. Interface declares methods which corresponds to required service.

**Component** is class which implements Interface. There can be more components implementing the same interface. E.g. component logging to file and component logging into database. Both can implement the same interface.  Component in Shape framework has a special interface which enables to get a list of provided and required interfaces from library. It can ve said that component implement a service through provided interface.

**ComponentInstance** is an object of given component created by component constructor. Number of created **ComponentInstance** depends on application configuration. E.g. file logging component can have three instances, one for critical errors, second for core and third for specific component.

**Data class** represents data and its collections and hierarchy. **Data classes** can be created or destroyed anytime and their number is not limited. They are handled via **ComponentInstances** and their hierarchy is defined by design itself. Implementation can be done as static or dynamic library or even in header file.

## 2 Naming convention

* Name of Interface always begins with **I** e.g. IChannel and contains pure virtual methods.
* Service class always ends with **Srvc** e.g. **ICncDataSrvc** and besides virtual methods also declares a service which implements component.
* Name of component implementation begins with **Cm** e.g. **CmCobaltData**.

## 3 Gateway scheme


## 4 Interfaces and Components

TODO: Basic component diagram.

## 5 Modules implementation

TODO
