#!/usr/bin/env python3

from typing import Dict, List, Set, Union
from pathlib import Path
import json

### Paths ###
CFG_DIR = '/etc/iqrf-gateway-daemon/'
SCHEDULER_TASK_DIR = '/var/cache/iqrf-gateway-daemon/scheduler/'
SPLITTER_FILE = 'iqrf__JsonSplitter.json'
SENSOR_DATA_FILE = 'iqrf__IqrfSensorData.json'

### Component names ###
MQTT_COMPONENT: str = 'iqrf::MqttMessaging'
WS_COMPONENT: str = 'iqrf::WebsocketMessaging'
SPI_COMPONENT: str = 'iqrf::IqrfSpi'

### Properties ###
COMPONENT_PROPERTY: str = 'component'
INSTANCE_PROPERTY: str = 'instance'
MESSAGING_LIST_PROPERTY: str = 'messagingList'
TASK_PROPERTY: str = 'task'
MESSAGING_PROPERTY: str = 'messaging'

### Helper functions ###

def update_instances(instances: List[Union[str, Dict]], mqtt: Set[str], ws: Set[str]):
  messagings = []
  for entry in instances:
    if isinstance(entry, str):
      if entry in mqtt:
        messagings.append({
          'type': 'mqtt',
          'instance': entry,
        })
      if entry in ws:
        messagings.append({
          'type': 'ws',
          'instance': entry,
        })
    if isinstance(entry, dict):
      keys = entry.keys()
      if len(keys) == 2 and 'type' in keys and isinstance(entry['type'], str) and 'instance' in keys and isinstance(entry['instance'], str):
        messagings.append(entry)
  return list(set([json.dumps(x, sort_keys=True) for x in messagings]))

### Update script ###

mqtt_instances: Set[str] = set()
ws_instances: Set[str] = set()

# iterate over config files to find component files and perform tasks
for cfg_file in Path(CFG_DIR).glob('*.json'):
  # open config file
  with cfg_file.open(mode='r', encoding='utf-8') as f:
    data = json.load(f)

  # remove config file if the configuration file is for removed IqrfSpi component
  component_name = data.get(COMPONENT_PROPERTY)
  if component_name == SPI_COMPONENT:
    print(f'Removing SPI configuration file {cfg_file.name}')
    cfg_file.unlink(missing_ok=True)
    continue

  # store mqtt instances
  if component_name == MQTT_COMPONENT:
    mqtt_instances.add(data.get(INSTANCE_PROPERTY))
    continue
  # store ws instances
  if component_name == WS_COMPONENT:
    ws_instances.add(data.get(INSTANCE_PROPERTY))

# update configuration files that use
for cfg_file in [SPLITTER_FILE, SENSOR_DATA_FILE]:
  # check that file exists
  file = Path(CFG_DIR) / cfg_file
  if not file.exists() or not file.is_file():
    continue

  # update messaging instances format
  with file.open(mode='r', encoding='utf-8') as f:
    data = json.load(f)
  messaging_list = update_instances(
    instances=data[MESSAGING_LIST_PROPERTY],
    mqtt=mqtt_instances,
    ws=ws_instances,
  )
  if len(messaging_list) == 0:
    continue
  data[MESSAGING_LIST_PROPERTY] = [json.loads(x) for x in messaging_list]
  # save file
  with file.open(mode='w', encoding='utf-8') as f:
    json.dump(data, f, ensure_ascii=False, separators=(',', ': '), indent=4)

# update scheduler task files messaging instances
for task_file in Path(SCHEDULER_TASK_DIR).glob('*.json'):
  with task_file.open(mode='r', encoding='utf-8') as f:
    data = json.load(f)
  # for each task, update its messaging format
  for task in data[TASK_PROPERTY]:
    messaging_list = update_instances(
      instances=task[MESSAGING_PROPERTY],
      mqtt=mqtt_instances,
      ws=ws_instances,
    )
    if len(messaging_list) == 0:
      continue
    task[MESSAGING_PROPERTY] = [json.loads(x) for x in messaging_list]
  # save updated task file
  with task_file.open(mode='w', encoding='utf-8') as f:
    json.dump(data, f, ensure_ascii=False, separators=(',', ': '), indent=4)
