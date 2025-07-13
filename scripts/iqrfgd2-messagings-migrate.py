#!/usr/bin/env python3

from pathlib import Path
import json

daemon_cfg_dir = '/etc/iqrf-gateway-daemon/'
daemon_scheduler_dir = '/var/cache/iqrf-gateway-daemon/scheduler/'

def get_updated_messagings(messagings):
  messaging_list = []
  for entry in messagings:
    if isinstance(entry, str):
      if entry in mqtt_instances:
        messaging_list.append({
          'type': 'mqtt',
          'instance': entry,
        })
      if entry in ws_instances:
        messaging_list.append({
          'type': 'ws',
          'instance': entry,
        })
    if isinstance(entry, dict):
      keys = entry.keys()
      if len(keys) == 2 and 'type' in keys and isinstance(entry['type'], str) and 'instance' in keys and isinstance(entry['instance'], str):
        messaging_list.append(entry)
  return list(set([json.dumps(x, sort_keys=True) for x in messaging_list]))

# get unique mqtt and ws instances
mqtt_instances = []
ws_instances = []
cfg_files = list(Path(daemon_cfg_dir).glob('*.json'))
for cfg_file in cfg_files:
  with cfg_file.open() as f:
    data = json.load(f)
  if 'component' not in data or 'instance' not in data:
    continue
  if data['component'] == 'iqrf::MqttMessaging':
    mqtt_instances.append(data['instance'])
  elif data['component'] == 'iqrf::WebsocketMessaging':
    ws_instances.append(data['instance'])
  else:
    continue
mqtt_instances = list(set(mqtt_instances))
ws_instances = list(set(ws_instances))

# update messaging instances in configuration files
for cfg_file in ['iqrf__JsonSplitter.json', 'iqrf__IqrfSensorData.json']:
  path = Path(daemon_cfg_dir) / cfg_file
  if not path.exists() or not path.is_file():
    continue
  with path.open(mode='r', encoding='utf-8') as f:
    data = json.load(f)
  messaging_list = get_updated_messagings(data['messagingList'])
  if len(messaging_list) == 0:
    continue
  data['messagingList'] = [json.loads(x) for x in messaging_list]
  with path.open(mode='w', encoding='utf-8') as f:
    json.dump(data, f, ensure_ascii=False, separators=(',', ': '), indent=4)

# update messaging instances in scheduler files
task_files = list(Path(daemon_scheduler_dir).glob('*.json'))
for task_file in task_files:
  with task_file.open(mode='r', encoding='utf-8') as f:
    data = json.load(f)
  for task in data['task']:
    messaging_list = get_updated_messagings(task['messaging'])
    if len(messaging_list) == 0:
      continue
    task['messaging'] = [json.loads(x) for x in messaging_list]
  with task_file.open(mode='w', encoding='utf-8') as f:
    json.dump(data, f, ensure_ascii=False, separators=(',', ': '), indent=4)
