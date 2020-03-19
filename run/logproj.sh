#!/bin/bash

POSTGRES_STRING='postgresql://'"$POSTGRES_USER"':'"$POSTGRES_PASSWORD"'@logproj-database:5432/'"$POSTGRES_DB"

cd /app
psql $POSTGRES_STRING < /app/sql/*.sql
mon "./logproj -d $POSTGRES_STRING"
