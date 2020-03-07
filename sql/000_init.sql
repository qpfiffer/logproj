BEGIN;

CREATE EXTENSION "uuid-ossp";

CREATE TABLE logproj."user" AS (
    id uuid DEFAULT public.uuid_generate_v4() NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE,
    updated_at TIMESTAMP WITH TIME ZONE,
    email_address TEXT NOT NULL,

    password TEXT NOT NULL,
    salt TEXT NOT NULL,
);

ALTER TABLE ONLY "user"
    ADD CONSTRAINT user_pkey PRIMARY KEY (id);

CREATE TRIGGER set_updated_at
    BEFORE UPDATE ON "logproj"."user"
    FOR EACH ROW EXECUTE PROCEDURE trigger_set_timestamp();

