#*******************************************************************************
#   Ledger App Hedera - Unit Tests
#   (c) 2024 Hedera Hashgraph
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#*******************************************************************************

# Unit test targets (no BOLOS SDK required)
.PHONY: build test clean help install-deps

# Default target
all: build

# Install dependencies (Ubuntu/Debian)
install-deps:
	@echo "Installing unit test dependencies..."
	@sudo apt-get update && sudo apt-get install -y cmake libcmocka-dev pkg-config

# Build unit tests
build:
	@echo "Building unit tests..."
	@mkdir -p build
	@cd build && cmake .. && make

# Run unit tests
test: build
	@echo "Running unit tests..."
	@cd build && CTEST_OUTPUT_ON_FAILURE=1 make test

# Clean unit test artifacts
clean:
	@echo "Cleaning unit tests..."
	@rm -rf build

# Help target
help:
	@echo "Available targets:"
	@echo "  build        - Build unit tests"
	@echo "  test         - Build and run unit tests"
	@echo "  clean        - Clean unit test artifacts"
	@echo "  install-deps - Install required dependencies (Ubuntu/Debian)"
	@echo "  help         - Show this help message" 