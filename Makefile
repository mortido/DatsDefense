DOCKER_IMAGE_NAME = dats_defense
DATA_DIR = $(CURDIR)/data

all: build run

build:
	docker build -t $(DOCKER_IMAGE_NAME) .

run:
	docker run --rm -v $(DATA_DIR):/app/data $(DOCKER_IMAGE_NAME)

attach:
	docker run --rm -it -v $(DATA_DIR):/app/data $(DOCKER_IMAGE_NAME) bash

stop:
	docker ps --filter ancestor=$(DOCKER_IMAGE_NAME) --format '{{.ID}}' | xargs -r docker stop
.PHONY: build run attach stop
