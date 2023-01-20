COMPONENT_SRCS=$(shell find ./components/ -iname "*.[c|h]")
MAIN_SRCS=$(shell find ./main/ -iname "*.[c|h]")

.PHONY: docs format fw

docs:
	doxygen .\Doxyfile

format:
	clang-format -i -style=file $(COMPONENT_SRCS) $(MAIN_SRCS)

fw:
	idf.py build