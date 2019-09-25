# It runs all tests.

from tavern.core import run

failure = run('tests.tavern.yaml', pytest_args=["-x"])

if failure:
    print("Error running tests.")
