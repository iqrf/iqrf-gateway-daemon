# It runs all tests.

from tavern.core import run

failure = run('tests_coordinator.tavern.yaml', pytest_args=["-x"])
failure |= run('tests_explore.tavern.yaml', pytest_args=["-x"])
failure |= run('tests_leds.tavern.yaml', pytest_args=["-x"])
failure |= run('tests_sensor.tavern.yaml', pytest_args=["-x"])

if failure:
    print("Error running tests.")
