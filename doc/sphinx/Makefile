# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXDIR     = .sphinx
SPHINXOPTS    ?= -c . -d $(SPHINXDIR)/.doctrees
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = .
BUILDDIR      = _build
VENVDIR       = $(SPHINXDIR)/venv
PA11Y         = $(SPHINXDIR)/node_modules/pa11y-ci/bin/pa11y-ci.js --config $(SPHINXDIR)/pa11y-ci.json
VENV          = $(VENVDIR)/bin/activate
WORKDIR	      = ../build_doc
WORKSPHINX    = $(WORKDIR)/doc/sphinx
PROJROOT      = ../..
DEPS	      = cmake doxygen libboost-dev libboost-iostreams-dev \
		libboost-filesystem-dev libboost-program-options-dev \
		libboost-system-dev libdrm-dev libegl-dev libepoxy-dev \
		libfreetype-dev libglib2.0-dev libgles2-mesa-dev \
		libglm-dev libinput-dev liblttng-ust-dev libwayland-dev \
		libxcb-composite0-dev libxcb-randr0-dev libxcursor-dev \
		libxkbcommon-dev libxml++2.6-dev libyaml-cpp-dev xsltproc

# If the environment variable `GITHUB_WORKFLOW` is empty, then we're running
# locally and shouldn't depend on `_build/index.html` to build HTML (so even if
# it exists, we'll rebuild)
# On CI where the envvar is set, we should depend on `_build/index.html` so we
# don't build multiple times (multiple invocations = multiple complete builds
# even if files didn't change)
ifeq ($(GITHUB_WORKFLOW), "")
HTMLINDEX     = ""
else
HTMLINDEX     = $(WORKSPHINX)/_build/index.html
endif

.PHONY: help full-help woke-install pa11y-install install run html epub serve \
        clean clean-doc spelling linkcheck woke pa11y Makefile

# Put it first so that "make" without argument is like "make help".
help:
	@echo "\n" \
        "--------------------------------------------------------------- \n" \
        "* watch, build and serve the documentation: make run \n" \
        "* only build: make html \n" \
        "* only serve: make serve \n" \
        "* clean built doc files: make clean-doc \n" \
        "* clean full environment: make clean \n" \
        "* check links: make linkcheck \n" \
        "* check spelling: make spelling \n" \
        "* check inclusive language: make woke \n" \
        "* check accessibility: make pa11y \n" \
        "* other possible targets: make <press TAB twice> \n" \
        "--------------------------------------------------------------- \n"

full-help: $(VENVDIR)
	@. $(VENV); $(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	@echo "\n\033[1;31mNOTE: This help texts shows unsupported targets!\033[0m"
	@echo "Run 'make help' to see supported targets."

configure:
	. $(VENV); \
	cmake $(PROJROOT) -B $(WORKDIR) -DMIR_ENABLE_TESTS=NO -DMIR_PLATFORM=x11;

# Shouldn't assume that venv is available on Ubuntu by default; discussion here:
# https://bugs.launchpad.net/ubuntu/+source/python3.4/+bug/1290847
$(SPHINXDIR)/requirements.txt:
	python3 $(SPHINXDIR)/build_requirements.py
	python3 -c "import venv" || sudo apt install python3-venv

# If requirements are updated, venv should be rebuilt and timestamped.
$(VENVDIR): $(SPHINXDIR)/requirements.txt
	@echo "... setting up virtualenv"
	python3 -m venv $(VENVDIR)
	. $(VENV); pip install --require-virtualenv \
	    --upgrade -r $(SPHINXDIR)/requirements.txt \
            --log $(VENVDIR)/pip_install.log
	@test ! -f $(VENVDIR)/pip_list.txt || \
            mv $(VENVDIR)/pip_list.txt $(VENVDIR)/pip_list.txt.bak
	@. $(VENV); pip list --local --format=freeze > $(VENVDIR)/pip_list.txt
	@touch $(VENVDIR)

woke-install:
	@type woke >/dev/null 2>&1 || \
            { echo "Installing \"woke\" snap... \n"; sudo snap install woke; }

pa11y-install:
	@type $(PA11Y) >/dev/null 2>&1 || { \
			echo "Installing \"pa11y-ci\" from npm... \n"; \
			mkdir -p $(SPHINXDIR)/node_modules/ ; \
			npm install --prefix $(SPHINXDIR) pa11y-ci; \
		}

install: $(VENVDIR)
	sudo apt install -y $(DEPS)

run: install configure
	. $(VENV); cd $(WORKSPHINX); sphinx-autobuild -b dirhtml "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS)

html: install configure
	cd $(WORKSPHINX); \
	. $(VENV); \
	$(SPHINXBUILD) -b dirhtml "$(SOURCEDIR)" "$(BUILDDIR)" -w $(SPHINXDIR)/warnings.txt $(SPHINXOPTS)

epub: install configure
	. $(VENV); cd $(WORKSPHINX); $(SPHINXBUILD) -b epub "$(SOURCEDIR)" "$(BUILDDIR)" -w $(SPHINXDIR)/warnings.txt $(SPHINXOPTS)

serve: html
	cd "$(BUILDDIR)"; python3 -m http.server 8000

clean: clean-doc
	@test ! -e "$(VENVDIR)" -o -d "$(VENVDIR)" -a "$(abspath $(VENVDIR))" != "$(VENVDIR)"
	rm -rf $(VENVDIR)
	rm -f $(SPHINXDIR)/requirements.txt
	rm -rf $(SPHINXDIR)/node_modules/
	rm -r $(WORKDIR)

clean-doc:
	git clean -fx "$(BUILDDIR)"
	rm -rf $(SPHINXDIR)/.doctrees

spelling: html
	. $(VENV) ; cd $(WORKSPHINX); python3 -m pyspelling -c $(SPHINXDIR)/spellingcheck.yaml -j $(shell nproc)

linkcheck: install
	. $(VENV) ; cd $(WORKSPHINX); $(SPHINXBUILD) -b linkcheck "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS)

# No need to go to $(WORKSPHINX) since it works on raw source files
woke: woke-install
	woke *.rst **/*.rst --exit-1-on-failure \
	    -c https://github.com/canonical/Inclusive-naming/raw/main/config.yml

pa11y: pa11y-install html
	$(PA11Y) $(shell find $(WORKSPHINX)/_build -name *.html -not -path '$(WORKSPHINX)/_build/api/*');

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	. $(VENV); $(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
