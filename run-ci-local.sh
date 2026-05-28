#!/bin/bash
set -e

echo "=========================================="
echo "Running local CI checks (mirrors .github/workflows/ci.yml)"
echo "=========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check formatting
echo -e "\n${YELLOW}[1/4] Checking code formatting (clang-format)...${NC}"
if ! command -v clang-format &> /dev/null; then
    echo -e "${RED}ERROR: clang-format not installed${NC}"
    echo "Install with: sudo apt-get install clang-format (Linux) or brew install clang-format (macOS)"
    exit 1
fi

if find src -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run --Werror --style=file; then
    echo -e "${GREEN}✓ Formatting check passed${NC}"
else
    echo -e "${RED}✗ Formatting check failed${NC}"
    echo "Fix with: clang-format -i src/**/*.{cpp,h}"
    exit 1
fi

# Static analysis
echo -e "\n${YELLOW}[2/4] Running static analysis (cppcheck)...${NC}"
if ! command -v cppcheck &> /dev/null; then
    echo -e "${RED}ERROR: cppcheck not installed${NC}"
    echo "Install with: sudo apt-get install cppcheck (Linux) or brew install cppcheck (macOS)"
    exit 1
fi

if cppcheck \
    --enable=warning,style,performance \
    --suppress=missingIncludeSystem \
    --suppress=unusedFunction \
    --inline-suppr \
    -DPROGMEM= \
    -DF_CPU=16000000 \
    --error-exitcode=1 \
    src/; then
    echo -e "${GREEN}✓ Static analysis passed${NC}"
else
    echo -e "${RED}✗ Static analysis failed${NC}"
    exit 1
fi

# Build
echo -e "\n${YELLOW}[3/4] Building firmware (uno)...${NC}"
if pio run -e uno; then
    echo -e "${GREEN}✓ Build passed${NC}"

    # Extract and display memory usage
    echo -e "\n${YELLOW}Memory Usage:${NC}"
    pio run -e uno 2>&1 | grep -E "RAM|Flash" || true
else
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi

# Tests
echo -e "\n${YELLOW}[4/4] Running unit tests (native)...${NC}"
if pio test -e native; then
    echo -e "${GREEN}✓ Tests passed${NC}"
else
    echo -e "${RED}✗ Tests failed${NC}"
    exit 1
fi

echo -e "\n${GREEN}=========================================="
echo "All CI checks passed! ✓"
echo "==========================================${NC}"
