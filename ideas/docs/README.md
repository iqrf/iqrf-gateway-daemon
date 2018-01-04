# Documents flow-down

![docs-flowdown.png](uml/docs-flowdown.png)

**Documents:**
- **[ProductSpecification](product-spec/product-spec.md)** - high level product spec (like marketing reqs., data-sheet, etc..).
- **[ApiRequirements](api-reqs/api-reqs.md)** - detail API requirements.
- **[SwRequirements](sw-reqs/sw-reqs.md)** - detail SW requirements.
- **ApiDesign** - UML and Doxygen design.
- **ApiTests** - testing of requirements.
- **[SwDesign](sw-design/sw-design.md)** - UML and Doxygen design.
- **SwSpecificationTests** - testing of requirements.

**Rules:**
- Product spec numbering **SPEC-01**
- SwRequirements numbering **DEM-01, -02, ...**
- ApiRequirements numbering **API-01, -02, ...**
- **Linking:**
  - Requirements to Issues
  - Design to Requirements & Issue
  - Use relative links as shown in example:
  ```
  - **DEM-01** Modular core. [SPEC_01](../product-spec/product-spec.md#1-software-specification), [#2](https://github.com/logimic/gateway-daemon/issues/2)

    Basic module management core.
  ```
