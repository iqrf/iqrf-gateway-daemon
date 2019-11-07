# It runs all tests.

from tavern.core import run

failure = run('tests-broadcast_raw.tavern.yaml', pytest_args=["-x"])
failure |= run('tests-node0_coordinator.tavern.yaml', pytest_args=["-x"])
failure |= run('tests-node0_leds.tavern.yaml', pytest_args=["-x"])
failure |= run('tests-node0_os.tavern.yaml', pytest_args=["-x"])
failure |= run('tests-node0_sfrc.tavern.yaml', pytest_args=["-x"])
failure |= run('tests-node1_explore.tavern.yaml', pytest_args=["-x"])
failure |= run('tests-node1_iqmesh.tavern.yaml', pytest_args=["-x"])
failure |= run('tests-node1_sensor.tavern.yaml', pytest_args=["-x"])
failure |= run('tests-node2_bo.tavern.yaml', pytest_args=["-x"])
failure |= run('tests-node2_iqmesh.tavern.yaml', pytest_args=["-x"])
failure |= run('tests-node2_sensor.tavern.yaml', pytest_args=["-x"])
failure |= run('tests-node3_light.tavern.yaml', pytest_args=["-x"])
failure |= run('tests-node4_dali.tavern.yaml', pytest_args=["-x"])

if failure:
    print("Error running tests.")
