# Software Requirements

#### **DEM-01** Modular core. [SPEC_01](../product-spec/product-spec.md#1-software-specification), [#6](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/6)
  * Basic module management core.
  * Modul je implementovaný jako samostatně kompilovatelná sdílená knihovna.
  * Modul se skládá z jedné nebo více component.
  * Komponenty jsou provázané interface třídami (pure virtual).
  * Komponenty jsou black box pro ostatní komponenty.

#### **DEM-02** DaemonController module implementation. [#7](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/7)

#### **DEM-03** Scheduler module implementation. [#9](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/9)

#### **DEM-04** BaseService module implementation. [#10](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/10)

#### **DEM-05** JsonSerializer module implementation. [#11](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/11)

#### **DEM-06** UdpMessaging module implementation. [#12](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/12)

#### **DEM-07** MqttMessaging module implementation. [#13](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/13)

#### **DEM-08** Messaging module implementation. [#14](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/14)

#### **DEM-09** StandardDevice module implementation.[#15](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/15)

#### **DEM-10** DaemonConfiguration module implementation. [#16](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/16)

#### **DEM-11** DaemonStatistic module implementation. [#17](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/17)

* RF channel, Access password, bonding, discovery (basic)
* Network test (Enumerace) - table, info about errors, suggestion for updates, saving the table (advanced)
* Backup/Restore - option to save/export
* OTA Uload - plugin, handler, OS+DPA update
* TR configuration Coordinator, TR configuration Nodes
* These services should be callable from cloud platforms - via MQTT...
