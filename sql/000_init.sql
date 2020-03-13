BEGIN;

CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

CREATE SCHEMA IF NOT EXISTS "logproj";
SET search_path TO 'logproj';

CREATE OR REPLACE FUNCTION logproj.trigger_set_timestamp()
RETURNS TRIGGER AS $$
BEGIN
      NEW.updated_at = NOW();
      RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- User stuff
CREATE TABLE IF NOT EXISTS logproj."user" (
    id uuid DEFAULT public.uuid_generate_v4() NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE,
    email_address TEXT NOT NULL,

    password TEXT NOT NULL, -- scrypt hash with salt
    CONSTRAINT "user_pkey" PRIMARY KEY ("id") NOT DEFERRABLE INITIALLY IMMEDIATE
);

DROP TRIGGER IF EXISTS set_updated_at ON "logproj"."user";
CREATE TRIGGER set_updated_at
    BEFORE UPDATE ON "logproj"."user"
    FOR EACH ROW EXECUTE PROCEDURE trigger_set_timestamp();

-- API Keys
CREATE TABLE IF NOT EXISTS logproj."api_key" (
    id uuid DEFAULT public.uuid_generate_v4() NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE,

    key TEXT NOT NULL,

    user_id uuid NOT NULL,
    CONSTRAINT "api_key_pkey" PRIMARY KEY ("id") NOT DEFERRABLE INITIALLY IMMEDIATE,
    CONSTRAINT "api_key_user_fk" FOREIGN KEY ("user_id") REFERENCES "logproj"."user" ("id")
);

DROP TRIGGER IF EXISTS set_updated_at ON "logproj"."api_key";
CREATE TRIGGER set_updated_at
    BEFORE UPDATE ON "logproj"."api_key"
    FOR EACH ROW EXECUTE PROCEDURE trigger_set_timestamp();

-- Projects
CREATE TABLE IF NOT EXISTS logproj."project" (
    id uuid DEFAULT public.uuid_generate_v4() NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE,

    name TEXT NOT NULL,

    user_id uuid NOT NULL,
    CONSTRAINT "project_pkey" PRIMARY KEY ("id") NOT DEFERRABLE INITIALLY IMMEDIATE,
    CONSTRAINT "project_user_fk" FOREIGN KEY ("user_id") REFERENCES "logproj"."user" ("id"),
    UNIQUE (name, user_id)
);

DROP TRIGGER IF EXISTS set_updated_at ON "logproj"."project";
CREATE TRIGGER set_updated_at
    BEFORE UPDATE ON "logproj"."project"
    FOR EACH ROW EXECUTE PROCEDURE trigger_set_timestamp();

-- Log entries
CREATE TABLE IF NOT EXISTS logproj."log_entry" (
    id uuid DEFAULT public.uuid_generate_v4() NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE,

    meta_data JSONB NULL,
    entry TEXT NOT NULL,

    project_id uuid NOT NULL,
    CONSTRAINT "log_entry_pkey" PRIMARY KEY ("id") NOT DEFERRABLE INITIALLY IMMEDIATE,
    CONSTRAINT "log_entry_project_fk" FOREIGN KEY ("project_id") REFERENCES "logproj"."project" ("id")
);

DROP TRIGGER IF EXISTS set_updated_at ON "logproj"."log_entry";
CREATE TRIGGER set_updated_at
    BEFORE UPDATE ON "logproj"."log_entry"
    FOR EACH ROW EXECUTE PROCEDURE trigger_set_timestamp();

COMMIT;
