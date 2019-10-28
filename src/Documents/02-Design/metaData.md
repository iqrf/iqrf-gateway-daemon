# MetaData component design

The MetaData component persistently maintaines meta data information assigned to nodes

## General features
Meta data can be assigned via API by user. It forms relations:

**nadr -> mid -> metaId -> metaData**

- **metaData** is general JSON object specified by JSON schema passed to component instance via configuration. It's up to user what is kept inside
- **metaId** is automatically generated unique key within daemon instance (installation) when metaData passed to dedicated IF
- **mid** IQRF TR module identification (factory assigned)
- **nadr** Is node address of bonded IQRF TR module with mid
- pair [metaId, metaData] can be moved to other mid 
- mid can be assigned to other nadr as a result of network reconfiguration

Relation **nadr -> mid** is not kept by **R2.1** and has to be explicitly set by user via dedicated temporary interface. It may change in later versions - the relation will be maintained automatically as a result of bonding procedures
 
## Data structures:
- `typedef json metaData`
- `typedef string metaId`
- `typedef uint32_t mid`
- `typedef uint16_t nadr`
- `map<metaId, metData> metaIdMetaDataMap`
- `map<mid, metaId> midMetaIdMap`
- `map<nadr, mid> nadrMidMap`

## Usecases

UIF - is User Interface

### 1. Create metaData
User wants to create metaData as JSON object
- user creates metaData
- user calls UIF createMetaData(metaData)
- UIF generates daemon instance unique metaId as metaData identification
- UIF validates metaData against schema from configuration
- UIF stores [metaId, metaData] to metaIdMetaDataMap
- UIF returns
  - success status and generated metaId
  - error "JSON validation" in case schema validation 

### 2. Get metaData by metaId
User wants to get metaData by metaId
- user calls UIF getMetaData(metaId)
- UIF finds [metaId, metaData] in metaIdMetaDataMap
- UIF returns
  - success status and assigned metaData
  - error status "metaId not exist" if metaId is not found 

### 3. Update metaData
User wants to update assigned metaData.
- user gets metaData by usecase (2)
- user edits metaData
- user calls UIF updateMetaData(metaId, metaData)
- UIF validates metaData against schema from configuration
- UIF updates [metaId, metaData] in metaIdMetaDataMap
- UIF returns
  - success status
  - error status "metaId not exist" if passed metaId is not found 
  - error "JSON validation" in case schema validation 

### 4. Remove metaData
User wants to remove metaData by metaId.
- user calls UIF to removeMetaData(metaId)
- UIF checks if metaId is assigned to any mid in midMetaIdMap
- UIF erase [metaId, metaData] from metaIdMetaDataMap
- UIF returns
  - success status
  - error status "metaId not exist" if metaId is not found 
  - error status "metaId assigned" and mid it is assigned to 

### 5. Assign metaData to mid
User wants to assign metaData to unassigned mid.
- user knows metaId from usecase (1)
- user calls UIF assignMidMetaData(mid metaId)
- UIF finds [metaId, metaData] in metaIdMetaDataMap
- UIF insert [mid, metaId] to midMetaIdMap
- UIF returns
  - success status
  - error status "duplicity assignment" if mid has already assigned metaData
  - error status "metaId not exist" if metaId is not found 
  - error status "metaId assigned" if metaId has been already assigned to other mid 
  - error status "mid not exist" if mid is not found 

### 6. Get metaData by mid
User wants to get metaData assigned to mid.
- user calls UIF to getMidMetaData(mid)
- UIF finds [mid, metaId] in midMetaIdMap
- UIF finds [metaId, metaData] in metaIdMetaDataMap
- UIF returns
  - success status and assigned [metaId, metaData]
  - error status "mid no metaId" if there's no metaId assigned to mid
  - error status "metaId inconsistent" if there's no [metaId, metaData] metaIdMetaDataMap

### 7. Release metaData from mid
User wants to release assigned metaData from mid.
- user calls UIF to releaseMidMetaData(mid)
- UIF erase [mid, metaId] from midMetaIdMap
- UIF returns
  - success status, 
  - error status "mid not exist" if mid is not found 
  - error status "mid no metaId" if there's no metaId assigned to mid

### 8. Get metaData by nadr
User wants to get metaData assigned to nadr.
- user calls UIF to getNadrMetaData(nadr)
- UIF finds [nadr, mid] in nadrMidMap
- UIF finds [mid, metaId] in midMetaIdMap
- UIF finds [metaId, metaData] in metaIdMetaDataMap
- UIF returns
  - success status and assigned [mid, metaId, metaData]
  - error status "nadr not exist" if there's no nadr
  - error status "nadr inconsistent" if there's no [nadr, mid] nadrMidMap
  - error status "mid no metaId" if there's no metaId assigned to mid
  - error status "metaId inconsistent" if there's no [metaId, metaData] metaIdMetaDataMap

### 9. Export metaData all
User wants to export complete midMetaIdMap, metaIdMetaDataMap tables
- user calls UIF exportMetaDataAll()
- UIF exports midMetaIdMap, metaIdMetaDataMap tables
- UIF returns
  - success status, exported tables

### 10. Verify metaData all
User wants to verify midMetaIdMap, metaIdMetaDataMap tables consistency
- user calls UIF verifyMetaDataAll()
- UIF verify midMetaIdMap, metaIdMetaDataMap tables
- UIF returns
  - success status consistent
  - error status "inconsistent" 
    - list of inconsistent mid - exists [mid, metaId] in midMetaIdMap but no [nadr, mid] in nadrMidMap 
    - list of inconsistent metaId - exists [mid, metaId] in midMetaIdMap but no [metaId, metaData] in metaIdMetaDataMap 

### 11. Import metaData all
User wants to import midMetaIdMap, metaIdMetaDataMap tables
- user calls UIF to importMetaDataAll(midMetaIdMap, metaIdMetaDataMap)
- UIF imports midMetaIdMap, metaIdMetaDataMap tables
- UIF returns
  - success status import
  - error status "inconsistent" 
    - list of inconsistent mid - exists [mid, metaId] in midMetaIdMap but no [nadr, mid] in nadrMidMap 
    - list of inconsistent metaId - exists [mid, metaId] in midMetaIdMap but no [metaId, metaData] in metaIdMetaDataMap 


### 12. Import nadrMidMap metaData (temporary for R2.1, nadrMidMap mainained automatically in later releases)
User wants to import nadrMidMap table
- user calls UIF importNadrMidMap(nadrMidMap)
- UIF returns
  - success status
  - error status "inconsistent" 
    - list of duplicity [nadr, mid] - exists duplicity nadr or mid, nadrMidMap records has to be 1:1    

### 13. Export nadrMidMap metaData (temporary for R2.1, nadrMidMap mainained automatically in later releases)
User wants to export nadrMidMap table
- user calls UIF exportNadrMidMap()
- UIF returns
  - success status

## JSON API Interface:

IF provide these request/response pairs descibed in dedicated sub-chapters

- **mngDaemon-setMetaData** to set metaId -> metaData relation
- **mngDaemon-getMetaData** to get metaId -> metaData relation
- **mngDaemon-setMidMetaId** to set mid -> metaId relation
- **mngDaemon-getMidMetaData** to get mid -> metaId -> metaData relation
- **mngDaemon-getNadrMetaData** to get nadr -> mid -> metaId -> metaData relation
- **mngDaemon-exportMetaDataAll** to export all mid -> metaId -> metaData relations
- **mngDaemon-verifyMetaDataAll** to verify all mid -> metaId -> metaData relations consistency
- **mngDaemon-importMetaDataAll** to import all mid -> metaId -> metaData relations
- **mngDaemon-importNadrMidMap** to import all nadr -> mid relations (temporary for R2.1)
- **mngDaemon-exportNadrMidMap** to export all nadr -> mid relations (temporary for R2.1)

JSON API represents invalid parameters in this way:
**metaData** as an empty JSON object: **{}**
**metaId** as an empty string: **""**
**mid** as int number: **-1** 
**nadr** as int number: **-1** 

### mngDaemon-setMetaData
Request with parameters:
- required **metaId: string**
- required **metaData: object** 

Possible parameters combination, parameter == 0 means invalid

| metaId | metaData | processing |
| ------ | ------ | ------ |
| 0 | 0 |  N/A |
| 0 | 1 | generate metaId, metaIdMetaDataMap.insert(metaId, metaData) |
| 1 | 0 | metaIdMetaDataMap.erase(metaId) |
| 1 | 1 | metaIdMetaDataMap.update(metaId, metaData) |

Response with parameters:
- required **metaId: string** original or generated metaId
- optional(verbose) **metaData: object** original metaData
- required **mid: integer** mid it is assigned to
- required **status**
  - ok
  - badparams   
  - metaIdUnknown
  - metaIdAssigned in case of erase request and it is assigned to mid 

### mngDaemon-getMetaData
Request with parameters:
- required **metaId: string**

Response with parameters:
- required **metaId: string** original metaId
- required **metaData: object** original metaData
- required **mid: integer** mid it is assigned to
- required **status**
  - ok
  - badParams
  - metaIdUnknown
  
### mngDaemon-setMidMetaId
Request with parameters:
- required **mid: integer**
- required **metaId: string**

Possible parameters combination, parameter == 0 means invalid

| mid | metaId | processing |
| ------ | ------ | ------ |
| 0 | 0 | N/A |
| 0 | 1 | N/A |
| 1 | 0 | midMetaIdMap.erase(mid) |
| 1 | 1 | midMetaIdMap.insert(mid, metaId) |

The pair [mid, metaId] has to be erased first if before `mid` gets assigned a another `metaId`

Response with parameters:
- required **mid: integer** original mid
- required **metaId: string** original metaId
- required **metaIdMid: integer** metaId assignment to mid
  - metaIdMid == mid in case of success update
  - metaIdMid != mid contains other mid if metaId is already assigned
- required **status**
  - ok
  - badParams
  - metaIdUnknown
  - metaIdAssigned in case of update request and it is assigned to other mid (in metaIdMid)
  - midAssigned

### mngDaemon-getMidMetaData
Request with parameters:
- required **mid: integer**

Response with parameters:
- required **mid: integer** original mid
- required **metaId: string** metaId assigned to mid
- required **metaData: object** metaData identified by metaId
- required **status**
  - ok
  - badParams 
  - midUnknown
  - metaIdInconsistent if there's no [metaId, metaData] in metaIdMetaDataMap
  
### mngDaemon-getNadrMetaData
Request with parameters:
- required **nadr: integer**

Response with parameters:
- required **nadr: integer** original nadr
- required **mid: integer** assigned mid
- required **metaId: string** metaId assigned to mid
- required **metaData: object** original metaData
- required **status**
  - ok
  - mataIdInconsistent if there's no [metaId, metaData] in metaIdMetaDataMap
  - midInconsistent if there's no metaId assigned to mid
  - nadrUnknown 

### mngDaemon-exportMetaDataAll
Request with parameters:
- N/A

Response with parameters:
- required **metaIdMetaDataMap: array**
  - items: **{ metaId: string, metaData: object }**  
- required **midMetaIdMap: array**
  - items: **{ mid: integer, metaId: string }**  
- required **status**
  - ok

### mngDaemon-verifyMetaDataAll
Request with parameters:
- N/A

Response with parameters:
- required **inconsistentMid: array**
  - items: **string**  exists [nadr, mid] in nadrMidMap but no [mid, metaId] in midMetaIdMap
- required **orphanedMid: array**
  - items: **string**  exists [mid, metaId] in midMetaIdMap but no [nadr, mid] in nadrMidMap
- required **inconsistentMetaId: array**
  - items: **string** exists [mid, metaId] in midMetaIdMap but no [metaId, metaData] in metaIdMetaDataMap
- required **orphanedMetaId: array**
  - items: **string** exists [metaId, metaData] in metaIdMetaDataMap but no [mid, metaId] in midMetaIdMap 
- required **status**
  - ok

### mngDaemon-importMetaDataAll
Request with parameters:
- required **metaIdMetaDataMap: array**
  - items: **{ metaId: string, metaData: object }**  
- required **midMetaIdMap: array**
  - items: **{ mid: integer, metaId: string }**  

Response with parameters:
- required **duplicitMetaId: array**
  - items: **string**  identified duplicit metaId as a key to be inserted to metaIdMetaDataMap
- required **duplicitMidMetaIdPair: array**
  - items: **string** identified duplicit pair [mid, metaId] to be inserted to metaIdMetaDataMap
- required **status**
  - ok
  - dulicitParam

### mngDaemon-importNadrMidMap (temporary R2.1)
Request with parameters:
- required **nadrMidMap: array**
  - items: **{ nadr: integer, mid: integer }**  

Response with parameters:
- required **duplicityNadrMid: array**
  - items: **{ nadr: integer, mid: integer }**  
- required **status**
  - ok
  - dulicitParam

### mngDaemon-exportNadrMidMap (temporary R2.1)
Request with parameters:
- N/A

Response with parameters:
- required **nadrMidMap: array**
  - items: **{ nadr: integer, mid: integer }**  
- required **status**
  - ok

### Usecases JSON API implementation

| num | usecase | usage |
| ------ | ------ | ------ |
| 1 | Create metaData | `mngDaemon-setMetaData: { metaId: "", metaData: {<data>} }` |
| 2 | Get metaData by metaId | `mngDaemon-getMetaData: { metaId: "<val>"}` |
| 3 | Update metaData | `mngDaemon-getMetaData: { metaId: "<val>" }` update and `mngDaemon-setMetaData: { metaId: "<val>", metaData: {<data>} }` |
| 4 | Remove metaData | `mngDaemon-setMetaData: { metaId: "", metaData: {} }` |
| 5 | Assign metaData to mid | `mngDaemon-setMidMetaId: { mid: <val>, metaId: "<val>" }` |
| 6 | Get metaData by mid | `mngDaemon-getMidMetaData: { mid: <val> }` |
| 7 | Release metaData from mid | `mngDaemon-setMidMetaId: { mid: <val>, metaId: "" }` |
| 8 | Get metaData by nadr | `mngDaemon-getNadrMetaData: { nadr: <val> }` |
| 9 | Export metaData all | `mngDaemon-exportMetaDataAll: {}` |
| 10 | Verify metaData all | `mngDaemon-verifyMetaDataAll: {}` |
| 11 | Import metaData all | `mngDaemon-importMetaDataAll: { metaIdMetaDataMap: [<vals>], midMetaIdMap: [<vals>] }` |
| 12 | Import nadrMidMap metaData | `mngDaemon-importNadrMidMap: {}` |
| 13 | Export nadrMidMap metaData | `mngDaemon-exportNadrMidMap: { nadrMidMap: [<vals>] }` |

