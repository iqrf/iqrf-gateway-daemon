# Software design [#18](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/18)


## 1 Component design

Software design is based on [Shape](https://github.com/logimic/shape) component framework.
The framework itself is just a glue to put all components together. However it allows flexible adding of other features or redesign existing.  The business logic itself is implemented in components described in this document.
The framework is described in: [Shape/README.md](https://github.com/logimic/shape/blob/master/README.md). It follows basic principals as defines by these terms used in next text

**Module** is distributable software in form of shared library. Includes components data classes and logically merges related SW parts.

**Interface** is pure virtual (abstract) class containing methods (functions). Used parameters should be general not exposing implementation details.

**Component** is class which implements Interface. There can be more components implementing the same Interface. E.g. component logging to file and component logging into database. Both can implement the same Interface. Component may use other Interfaces implemented by other Components.

**Component Instance** is an object of a component created by the component constructor. Number of created Component Instances depends on an application configuration. E.g. file logging component can have three instances, one for critical errors, second for debug level and third for a specific component.

**Provided Interface** is running Component Instance implementing the Interface published to be used.

**Required Interface** is published placeholder for required Interface. If a Component Instance wants to use an Interface it publish the placeholder.

**Attach Interface** is delivering Provided Interface to Required Interface placeholder.

**Detach Interface** is withdrawal of Provided Interface to Required Interface placeholder.

**Interface Cardinality** is SINGLE or MULTIPLE. It means Required Interface instance can be attached just once or all instantiated Interfaces can be attached.

**Interface Optionality** is UNREQUIRED or MANDATORY. It means all MANDATORY Interfaces have to be attached.

**Activate** is a Component Instance state when all MANDATORY Required Interfaces were attached and the instance is set by Shape to normal operation.

**Dectivate** is a Component Instance state when some of MANDATORY Required Interface was detached and the instance is set by Shape to stop normal operation.

**Service** is represented by its Interface. Provides service like doing command, send data, parse data, calculate data, registering call-back, etc. Service is a published Interface (Provided or Required) within Shape framework.

## 3 Gateway scheme

TODO

## 4 Interfaces and Components

### Components with respect to Async API


## 5 Modules implementation

TODO
