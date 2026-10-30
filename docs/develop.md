# Building and testing

## Repo layout

| Path | What |
|------|------|
| `format/schema.json` | The file format, as JSON Schema. |
| `format/validate.py` | Checks `.beat` files against the schema. |
| `tests/data/*.beat` | Example and test games. |
| `docs/` | This site (MkDocs). |

The C++ core and the players are added in later milestones. See the
[roadmap](roadmap.md).

## Check the example games

```bash
python -m venv .venv
. .venv/bin/activate
pip install -r requirements-docs.txt
python format/validate.py tests/data/*.beat
```

## Build the docs

```bash
mkdocs serve          # live preview at http://127.0.0.1:8000
mkdocs build --strict # what CI runs
```

## CI

`.github/workflows/ci.yml` runs on every pull request and on `main`. It checks
the example games against the schema and builds the docs with `--strict`.
