#!/bin/sh

cd /app
mon "./logproj -d postgresql://logproj:localpw@logproj-database:5432/logproj"
