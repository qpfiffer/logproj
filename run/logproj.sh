#!/bin/bash

POSTGRES_STRING='postgresql://logproj:localpw@logproj-database:5432/logproj'

cd /app
psql $POSTGRES_STRING < /app/sql/*.sql
mon "./logproj -d $POSTGRES_STRING"
