# Installation

## Without Docker

You'll need a few dependencies, get them whereever dependencies are sold, or
your favorite podcasting application:

* libjwt
* libscrypt
* [38-Moths](https://github.com/qpfiffer/38-Moths)
* libpq (the postgresql library)

Once those are all installed, you can just run `make` and you'll end up with a
`logproj` binary.

## With Docker

```bash
./docker_build.sh
```

This will give you an image tagged with `logproj`

### Docker Compose

`docker-compose up` will give you a working system, once you've built the
prerequisites.

You'll have to fill in the database schema, which brings us to...

# Database Setup

If you're running with docker-compose, just do this:

```Bash
psql --host localhost --port 8001 --user logproj < ./sql/000_init.sql
```

Do whatever is similar if you're not doing that, ie. dump your schema into the
database with psql and that initial file.
