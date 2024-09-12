import json
import os
import pytest
import sys

from jsonschema import Draft7Validator
from referencing import Registry, Resource
from referencing.jsonschema import DRAFT7
from typing import List, Tuple

from jsonschema.exceptions import ValidationError
from referencing.exceptions import NoSuchResource

# Go to the Schema base directory
os.chdir( os.path.join(os.path.dirname(__file__), '..', '..', '..', 'daemon', 'api') )

# Paths to the examples and schema directories
SCHEMA_DIR: str = ''
EXAMPLE_DIR: str = 'examples'

def get_example_json(name: str) -> dict:
    """Loads the example JSON from the appropriate directory"""
    path = os.path.join(EXAMPLE_DIR, f"{name}-example.json")
    with open(path, "r", encoding='utf-8') as file:
        return json.load(file)

def get_schema(name: str) -> dict:
    """Loads the JSON schema from the appropriate directory"""
    path = os.path.join(SCHEMA_DIR, f"{name}.json")
    with open(path, "r", encoding='utf-8') as file:
        return json.load(file)

examples: List[str] = [file.replace('-example.json', '') for file in os.listdir(EXAMPLE_DIR) if file.endswith('-example.json')]

def retrieve_from_filesystem(path: str):
    full_path = os.path.join(SCHEMA_DIR, path)
    if not os.path.exists(full_path):
        raise NoSuchResource(ref=path)
    with open(full_path, "r", encoding='utf-8') as file:
        return DRAFT7.create_resource(json.load(file))

registry = Registry(retrieve=retrieve_from_filesystem)

@pytest.mark.parametrize("example", examples)
def test_validation(example: str) -> None:
    data = get_example_json(example)
    schema = get_schema(example)
    validator = Draft7Validator(schema=schema, registry=registry)

    if not isinstance(data, list):
        data = [data]

    for case in data:

        err_iter = validator.iter_errors(case)

        errors = ''
        for error in err_iter:
            errors += f"{error.message}\n"

        if len(errors) > 0:
            pytest.fail(f"{errors}\n\nExample JSON:\n{json.dumps(data, indent=2)}", pytrace=False)

