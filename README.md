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

# Database Setup
