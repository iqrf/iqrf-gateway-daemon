from pathlib import Path
import json

daemon_cfg_dir = '/etc/iqrf-gateway-daemon/'

cfg_files = list(Path(daemon_cfg_dir).glob('*.json'))

mqtt_instances = []
ws_instances = []

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

for cfg_file in ['iqrf__JsonSplitter.json', 'iqrf__IqrfSensorData.json']:
    path = Path(daemon_cfg_dir) / cfg_file
    if not path.exists() or not path.is_file():
        continue
    with path.open(mode='r', encoding='utf-8') as f:
        data = json.load(f)
    current_messaging_list = list(data['messagingList'])
    new_messaging_list = []
    if len(current_messaging_list) == 0:
        continue
    for entry in current_messaging_list:
        if isinstance(entry, str):
            if entry in mqtt_instances:
                new_messaging_list.append({
                    'type': 'mqtt',
                    'instance': entry
                })
            if entry in ws_instances:
                new_messaging_list.append({
                    'type': 'ws',
                    'instance': entry
                })
        if isinstance(entry, dict):
            keys = entry.keys()
            if len(keys) == 2 and 'type' in keys and isinstance(entry['type'], str) and 'instance' in keys and isinstance(entry['instance'], str):
                new_messaging_list.append(entry)
            else:
                continue
    sorted_unique_list = list(set([json.dumps(x, sort_keys=True) for x in new_messaging_list]))
    data['messagingList'] = [json.loads(x) for x in sorted_unique_list]
    with path.open(mode='w', encoding='utf-8') as f:
        json.dump(data, f, ensure_ascii=False, separators=(',', ': '), indent=2)
