#!/bin/bash

cd $(dirname $(readlink -f $0))

mkdir -p build/bin/models

MODEL_NAME=sv-small-q3_k.gguf

wget -O ${MODEL_NAME} 'https://huggingface.co/lovemefan/sense-voice-gguf/resolve/main/sense-voice-small-q3_k.gguf?download=true'
mv ${MODEL_NAME} build/bin/models/

