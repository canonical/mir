import datetime
import os
import yaml

import subprocess
import re
from pathlib import Path

# Configuration for the Sphinx documentation builder.
# All configuration specific to your project should be done in this file.
#
# If you're new to Sphinx and don't want any advanced or custom features,
# just go through the items marked 'TODO'.
#
# A complete list of built-in Sphinx configuration values:
# https://www.sphinx-doc.org/en/master/usage/configuration.html
#
# Our starter pack uses the custom Canonical Sphinx extension
# to keep all documentation based on it consistent and on brand:
# https://github.com/canonical/canonical-sphinx


#######################
# Project information #
#######################

# Project name

project = "Mir"
author = "Canonical Ltd."


# Sidebar documentation title; best kept reasonably short
#
# The title you want to display for the documentation in the sidebar.
# You might want to include a version number here.
# To not display any title, set this option to an empty string.
try:
    release = subprocess.check_output(["git", "describe", "--always"], encoding="utf-8").strip()
except (FileNotFoundError, subprocess.CalledProcessError):
    with open(Path(__file__).parents[2] / "CMakeLists.txt", encoding="utf-8") as cmake:
        matches = re.findall(r"set\(MIR_VERSION_(MAJOR|MINOR|PATCH) (\d+)\)", cmake.read())
    release = "v{MAJOR}.{MINOR}.{PATCH}".format(**dict(matches))
html_title = f'{project} {release} documentation'


# Copyright string; shown at the bottom of the page
#
# Now, the starter pack uses CC-BY-SA as the license
# and the current year as the copyright year.
#
# NOTE: For static works, it is common to provide the first publication year.
#       Another option is to provide both the first year of publication
#       and the current year, especially for docs that frequently change,
#       e.g. 2022–2023 (note the en-dash).
#
#       A way to check a repo's creation date is to get a classic GitHub token
#       with 'repo' permissions; see https://github.com/settings/tokens
#       Next, use 'curl' and 'jq' to extract the date from the API's output:
#
#       curl -H 'Authorization: token <TOKEN>' \
#         -H 'Accept: application/vnd.github.v3.raw' \
#         https://api.github.com/repos/canonical/<REPO> | jq '.created_at'
copyright = '%s, %s' % (datetime.date.today().year, author)


# Documentation website URL
#
# NOTE: The Open Graph Protocol (OGP) enhances page display in a social graph
#       and is used by social media platforms; see https://ogp.me/
ogp_site_url = "https://canonical-mir.readthedocs-hosted.com/"


# Preview name of the documentation website
ogp_site_name = project


# Preview image URL
ogp_image = "https://assets.ubuntu.com/v1/253da317-image-document-ubuntudocs.svg"


# Product favicon; shown in bookmarks, browser tabs, etc.
html_favicon = 'favicon.ico'


# Dictionary of values to pass into the Sphinx context for all pages:
# https://www.sphinx-doc.org/en/master/usage/configuration.html#confval-html_context

html_context = {
    # Product page URL; can be different from product docs URL
    "product_page": "https://canonical.com/mir",

    # Product tag image; the orange part of your logo, shown in the page header
    'product_tag': '_static/tag.png',

    # Your Discourse instance URL
    'discourse': 'https://discourse.ubuntu.com/c/project/mir/15',

    # Your Mattermost channel URL
    "mattermost": '',

    # Your Matrix channel URL
    "matrix": "https://matrix.to/#/#mir-server:matrix.org",

    # Your documentation GitHub repository URL
    "github_url": "https://github.com/canonical/mir",

    # Docs branch in the repo; used in links for viewing the source files
    'repo_default_branch': 'main',

    # Docs location in the repo; used in links for viewing the source files
    "repo_folder": "/doc/sphinx/",

    # To enable or disable the Previous / Next buttons at the bottom of pages
    # Valid options: none, prev, next, both
    # "sequential_nav": "both",
    "display_contributors": False,

    # Required for feedback button    
    'github_issues': 'enabled',
}

html_theme_options = {
    'source_edit_link': 'https://github.com/canonical/mir',
}

# Project slug; see https://meta.discourse.org/t/what-is-category-slug/87897
slug = ''

#######################
# Sitemap configuration: https://sphinx-sitemap.readthedocs.io/
#######################

# Base URL of RTD hosted project

html_baseurl = 'https://canonical-starter-pack.readthedocs-hosted.com/'

# URL scheme. Add language and version scheme elements.
# When configured with RTD variables, check for RTD environment so manual runs succeed:

if 'READTHEDOCS_VERSION' in os.environ:
    version = os.environ["READTHEDOCS_VERSION"]
    sitemap_url_scheme = '{version}{link}'
else:
    sitemap_url_scheme = 'MANUAL/{link}'

# Include `lastmod` dates in the sitemap:

sitemap_show_lastmod = True

#######################
# Template and asset locations
#######################

#html_static_path = ["_static"]
#templates_path = ["_templates"]


#############
# Redirects #
#############

# To set up redirects: https://documatt.gitlab.io/sphinx-reredirects/usage.html
# For example: 'explanation/old-name.html': '../how-to/prettify.html',
# To set up redirects in the Read the Docs project dashboard:
# https://docs.readthedocs.io/en/stable/guides/redirects.html
# NOTE: If undefined, set to None, or empty,
#       the sphinx_reredirects extension will be disabled.
redirects = {
    'how-to/how-to-enable-graphics-core22-on-a-device': '../how-to-enable-graphics-for-snaps-on-a-device',
    'explanation/ok-so-what-is-this-wayland-thing-anyway': '../../how-to/developing-a-wayland-compositor-using-mir'
}


###########################
# Link checker exceptions #
###########################

# A regex list of URLs that are ignored by 'make linkcheck'
linkcheck_ignore = [
    "http://127.0.0.1:8000",
    "how-to/getting_involved_in_mir"
    ]


# A regex list of URLs where anchors are ignored by 'make linkcheck'
linkcheck_anchors_ignore_for_url = [
    "https://docs.google.com/*",
    "https://github.com/*",
    "https://manpages.ubuntu.com/*",
    "https://matrix.to*",
]

# give linkcheck multiple tries on failure
linkcheck_timeout = 60
linkcheck_retries = 3

########################
# Configuration extras #
########################

# Custom MyST syntax extensions; see
# https://myst-parser.readthedocs.io/en/latest/syntax/optional.html
#
# NOTE: By default, the following MyST extensions are enabled:
#       substitution, deflist, linkify
myst_enable_extensions = set()


# Custom Sphinx extensions; see
# https://www.sphinx-doc.org/en/master/usage/extensions/index.html
# NOTE: The canonical_sphinx extension is required for the starter pack.
#       It automatically enables the following extensions:
#       - custom-rst-roles
#       - myst_parser
#       - notfound.extension
#       - related-links
#       - sphinx_copybutton
#       - sphinx_design
#       - sphinx_reredirects
#       - sphinx_tabs.tabs
#       - sphinxcontrib.jquery
#       - sphinxext.opengraph
#       - terminal-output
#       - youtube-links

extensions = [
    "canonical_sphinx",
    "sphinxcontrib.cairosvgconverter",
    "sphinx_last_updated_by_git",
    "sphinx.ext.intersphinx",
    "sphinx_sitemap",
    'sphinx.ext.autodoc',
    'sphinx.ext.doctest',
    'sphinx.ext.mathjax',
    'sphinx.ext.viewcode',
    'sphinx.ext.imgmath',
    'sphinx.ext.todo',
    'breathe',
    'exhale',
    'sphinx.ext.graphviz',
    'sphinxcontrib.mermaid'
]

# Excludes files or directories from processing

exclude_patterns = [
    "doc-cheat-sheet*",
]

# Adds custom CSS files, located under 'html_static_path'
html_css_files = []


# Adds custom JavaScript files, located under 'html_static_path'
html_js_files = []

# Feedback button at the top; enabled by default
disable_feedback_button = False


# Specifies a reST snippet to be prepended to each .rst file
# This defines a :center: role that centers table cell content.
# This defines a :h2: role that styles content for use with PDF generation.

rst_prolog = """
.. role:: center
   :class: align-center
.. role:: h2
    :class: hclass2
.. role:: woke-ignore
    :class: woke-ignore
.. role:: vale-ignore
    :class: vale-ignore
"""

# Workaround for https://github.com/canonical/canonical-sphinx/issues/34

if "discourse_prefix" not in html_context and "discourse" in html_context:
    html_context["discourse_prefix"] = html_context["discourse"] + "/t/"

# Workaround for substitutions.yaml

if os.path.exists('./reuse/substitutions.yaml'):
    with open('./reuse/substitutions.yaml', 'r') as fd:
        myst_substitutions = yaml.safe_load(fd.read())

# Add configuration for intersphinx mapping

intersphinx_mapping = {
    'starter-pack': ('https://canonical-example-product-documentation.readthedocs-hosted.com/en/latest', None)
}

############################################################
### Additional configuration
############################################################
cmake_build_dir = Path('../../')  # Leave doc/sphinx
sphinx_dir = cmake_build_dir / 'doc/sphinx'

primary_domain = 'cpp'

highlight_language = 'cpp'



def read_doxyfile(path):
    # Instead of configuring Mir again to get the doxyfile, we assume the user
    # already has configured it and grab the file from there.
    try:
        # Read the doxyfile from the build directory
        with open(path) as doxyfile_file:
            return doxyfile_file.read()
    except IOError as e:
        raise Exception(f"""IOError: {e.errno}, {e.strerror}
                        \rHint: Have you configured the cmake project and changed `cmake_build_dir` to point at it?""")


doxyfile_contents = read_doxyfile(cmake_build_dir / 'doc/sphinx/Doxyfile')

# Setup the exhale extension
exhale_args = {
    # These arguments are required
    "containmentFolder": "./api",
    "rootFileName": "library_root.rst",
    "doxygenStripFromPath": "..",
    # Heavily encouraged optional argument (see docs)
    "rootFileTitle": "Mir API",
    # Suggested optional arguments
    "createTreeView": False,
    "exhaleExecutesDoxygen": True,
    "exhaleUseDoxyfile": False,
    "contentsDirectives": False,
    "exhaleDoxygenStdin": doxyfile_contents
}

# Setup the breathe extension
breathe_projects = {"Mir": "./xml/"}
breathe_default_project = "Mir"
breathe_default_members = ('members', 'undoc-members')
breathe_order_parameters_first = True

# Mermaid
mermaid_version = "10.5.0"
mermaid_d3_zoom = True

# MyST
myst_heading_anchors = 3
