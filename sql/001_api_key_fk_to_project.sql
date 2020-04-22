BEGIN;

SET search_path TO 'logproj';

ALTER TABLE logproj."api_key" ADD COLUMN "project_id" uuid;
ALTER TABLE logproj."api_key" ADD CONSTRAINT "api_key_project_fk" FOREIGN KEY ("project_id") REFERENCES logproj."project"(id);

COMMIT;
