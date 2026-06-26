import datetime
import os
import textwrap

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
# The Sphinx Stack uses the Canonical Sphinx theme to keep all documentation consistent
# and on brand:
# https://github.com/canonical/canonical-sphinx

#######################
# Project information #
#######################

# Project name
# TODO: Update with the official name of your project or product (e.g., "Ubuntu Server")
project = "Mir"

try:
    release = subprocess.check_output(
        ["git", "describe", "--tags", "--abbrev=0"], encoding="utf-8"
    ).strip()
except (FileNotFoundError, subprocess.CalledProcessError):
    with open(Path(__file__).parents[2] / "CMakeLists.txt", encoding="utf-8") as cmake:
        match = re.search(r"^\s*VERSION (\d+\.\d+\.\d+)$", cmake.read())
    release = f"v{match.group(1)}" if match else "unknown"

# Author name; used in the default copyright statement in the page footer
author = "Canonical Ltd."
version = os.getenv("READTHEDOCS_VERSION", "local")

# The year in the copyright statement
copyright = f"{datetime.date.today().year}"

# Sidebar documentation title
# To disable the title, set it to an empty string.
html_title = f"{project} {release} documentation"

# Documentation website URL
ogp_site_url = f"https://canonical.com/mir/docs/{version}"

# Preview name of the documentation website
# TODO: To use a different name for the project in previews, update the next line.
ogp_site_name = project

# Preview image URL
# TODO: To customise the preview image, update the next line.
ogp_image = "https://assets.ubuntu.com/v1/cc828679-docs_illustration.svg"

# Product favicon; shown in bookmarks, browser tabs, etc.
# TODO: To customise the favicon, uncomment and update the next line.
html_favicon = "favicon.ico"

# Dictionary of values to pass into the Sphinx context for all pages:
# https://www.sphinx-doc.org/en/master/usage/configuration.html#confval-html_context
html_context = {
    # Product page URL; can be different from product docs URL
    # TODO: Change to your product website URL, dropping the 'https://' prefix (e.g.,
    #       'ubuntu.com/lxd'). If there's no such website, remove the {{ product_page }}
    #       link from the _templates/header.html file.
    "product_page": "canonical.com/mir",
    # Product tag image; the orange part of your logo, shown in the page header
    # TODO: To add a tag image, uncomment and update as needed.
    # 'product_tag': '_static/tag.png',
    # Your Discourse instance URL
    # TODO: Change to your Discourse instance URL or leave empty.
    "discourse": "https://discourse.ubuntu.com/c/project/mir/15",
    # Your Mattermost channel URL
    # TODO: Change to your Mattermost channel URL or leave empty.
    "mattermost": "https://chat.canonical.com/canonical/channels/mir-internal",
    # Your Matrix channel URL
    # TODO: Change to your Matrix channel URL or leave empty.
    "matrix": "https://matrix.to/#/#mir-server:matrix.org",
    # Your documentation GitHub repository URL If set, links for viewing the
    # documentation source files and creating GitHub issues are added at the bottom of
    # each page.
    # TODO: Change to your documentation GitHub repository URL or leave empty.
    "github_url": "https://github.com/canonical/mir",
    # Docs branch in the repo; used in links for viewing the source files
    "repo_default_branch": "main",
    # Docs location in the repo; used in links for viewing the source files
    "repo_folder": "/doc/sphinx/",
    # TODO: To enable or disable the Previous / Next buttons at the bottom of pages
    # Valid options: none, prev, next, both
    # "sequential_nav": "both",
    # TODO: To enable listing contributors on individual pages, set to True
    "display_contributors": False,
    # Required for feedback button
    "github_issues": "enabled",
    # Passes the top-level 'author' value to the theme
    "author": author,
    # Documentation license information
    "license": {
        # TODO: Specify your project's license.
        # For the name, we recommend using the standard shorthand identifier from
        # https://spdx.org/licenses
        "name": "GPL-3.0-only AND GPL-2.0-only AND LGPL-3.0-only AND LGPL-2.0-only",
        # TODO: Link directly to your project's license statement.
        "url": "https://github.com/canonical/mir#copyright-and-license",
    },
}

html_extra_path = ["robots.txt", "sitemapindex.xml"]

# TODO: To enable the edit button on pages, uncomment and change the link to a
# public repository on GitHub or Launchpad. Any of the following link domains
# are accepted:
# - https://github.com/example-org/example"
# - https://launchpad.net/example
# - https://git.launchpad.net/example
#
html_theme_options = {
    'source_edit_link': 'https://github.com/canonical/mir',
}

# Project slug
# TODO: If your documentation is hosted on https://documentation.ubuntu.com/,
#       uncomment and set to the RTD slug.
slug = "mir/docs"

#######################
# Sitemap configuration: https://sphinx-sitemap.readthedocs.io/
#######################

# Use RTD canonical URL to ensure duplicate pages have a specific canonical URL
html_baseurl = f"https://canonical.com/mir/docs/{version}/"

# sphinx-sitemap uses html_baseurl to generate the full URL for each page:
sitemap_url_scheme = "{link}"

# Include `lastmod` dates in the sitemap:
sitemap_show_lastmod = True

# TODO: Exclude pages that aren't user-facing from the sitemap (e.g., module pages
# generated by autodoc).
# Pages excluded from the sitemap:
sitemap_excludes = [
    "404/",
    "genindex/",
    "search/",
]

################################
# Template and asset locations #
################################

html_static_path = ["_static"]
templates_path = ["_templates"]

#############
# Redirects #
#############

# Add redirects to the 'redirects.txt' file
# https://sphinxext-rediraffe.readthedocs.io/en/latest/

# To set up redirects in the Read the Docs project dashboard:
# https://docs.readthedocs.io/en/stable/guides/redirects.html

rediraffe_redirects = "redirects.txt"

# Strips '/index.html' from destination URLs when building with 'dirhtml'
rediraffe_dir_only = True


############################
# sphinx-llm configuration #
############################

# This description is included in llms.txt to provide some initial context for your
# product docs.
# TODO: Add a description in the form "This is the documentation for <product name>,
# <first sentence of home page>".
llms_txt_description = textwrap.dedent(
    """\
    This is the documentation for Mir, the set of libraries to build Wayland compositors.
    """
)

# The base URL for references built by sphinx-markdown-builder.
if os.environ.get("READTHEDOCS"):
    markdown_http_base = html_baseurl

###########################
# Link checker exceptions #
###########################

# A regex list of URLs that are ignored by 'make linkcheck'
linkcheck_ignore = [
    "http://127.0.0.1:8000",
    "https://github.com",
    r"https://matrix\.to/.*",
    "https://example.com",
    # SourceForge domains often block linkcheck
    r"https://.*\.sourceforge\.(net|io)/.*",
    # Khronos extension registry blocks some automated linkcheck requests.
    r"https://registry\.khronos\.org/EGL/extensions/.*",
    "how-to/getting_involved_in_mir",
    "https://www.mail-archive.com",
    "https://manpages.ubuntu.com",
]

# A regex list of URLs where anchors are ignored by 'make linkcheck'
linkcheck_anchors_ignore_for_url = [
    r"https://github\.com/.*",
    "https://docs.google.com/*",
    "https://manpages.ubuntu.com/*",
    "https://matrix.to*",
    "https://ticsdocumentation.tiobe.com/*",
    "https://community.nxp.com/*",
]

# How long the link checker will wait for a response for each request
# TODO: Decrease to improve run time or increase if links frequently time out.
# linkcheck_timeout = 60

# Give linkcheck multiple tries on failure
linkcheck_retries = 3

########################
# Configuration extras #
########################

# Custom MyST syntax extensions; see
# https://myst-parser.readthedocs.io/en/latest/syntax/optional.html
#
# NOTE: By default, the following MyST extensions are enabled:
#   - substitution
#   - deflist
#   - linkify
myst_enable_extensions = set(
    (
        "attrs_block",
        "attrs_inline",
    )
)

# Custom Sphinx extensions; see
# https://www.sphinx-doc.org/en/master/usage/extensions/index.html
extensions = [
    "canonical_sphinx",
    "notfound.extension",
    "sphinx_design",
    "sphinx_rerediraffe",
    "sphinx_reredirects",
    "sphinx_tabs.tabs",
    "sphinxcontrib.jquery",
    "sphinxext.opengraph",
    "sphinx_config_options",
    "sphinx_contributor_listing",
    "sphinx_filtered_toctree",
    "sphinx_llm.txt",
    "sphinx_related_links",
    "sphinx_roles",
    "sphinx_terminal",
    "sphinx_ubuntu_images",
    "sphinx_youtube_links",
    "sphinxcontrib.cairosvgconverter",
    "sphinx_last_updated_by_git",
    "sphinx.ext.intersphinx",
    "sphinx_sitemap",
    "sphinx.ext.autodoc",
    "sphinx.ext.doctest",
    "sphinx.ext.mathjax",
    "sphinx.ext.viewcode",
    "sphinx.ext.imgmath",
    "sphinx.ext.todo",
    "sphinx.ext.graphviz",
    "sphinxcontrib.mermaid",
]

# Excludes files or directories from processing
exclude_patterns = [
    "doc-cheat-sheet*",
    ".venv*",
]

# Adds custom CSS files, located remotely or in 'html_static_path'.
html_css_files = [
    "https://assets.ubuntu.com/v1/d86746ef-cookie_banner.css",
]

# Adds custom JavaScript files, located remotely or in 'html_static_path'.
html_js_files = [
    "js/overwrite_links.js",
    "https://assets.ubuntu.com/v1/287a5e8f-bundle.js",
]

# Appends extra markup to the end of every document written in reST
# rst_epilog = """
# """

# Feedback button at the top; enabled by default
# TODO: Disable the button if your project is unsuitable for public feedback.
# disable_feedback_button = True

# Your manpage URL
# TODO: To enable manpage links, uncomment and replace {codename} with required
#       release, preferably an LTS release (e.g. noble). Do *not* substitute
#       {section} or {page}; these will be replaced by sphinx at build time
#
# NOTE: If set, adding ':manpage:' to an .rst file
#       adds a link to the corresponding man section at the bottom of the page.
# manpages_url = 'https://manpages.ubuntu.com/manpages/{codename}/en/' + \
#     'man{section}/{page}.{section}.html'

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

# Configuration for Intersphinx projects
#
intersphinx_mapping = {
    "frame": ("https://ubuntu.com/frame/docs/24", None),
    "checkbox": ("https://canonical-checkbox.readthedocs-hosted.com/latest/", None),
    "server": ("https://documentation.ubuntu.com/server/", None),
    "snapcraft": ("https://documentation.ubuntu.com/snapcraft/stable/", None),
}

############################################################
### Additional configuration
############################################################
primary_domain = "cpp"
highlight_language = "cpp"


build_api_docs = os.getenv("MIR_BUILD_API_DOCS") == "1"

if build_api_docs:
    # Only add exhale and breathe if the user ran doxygen (which produces `<build_dir>/doc/doxygen_output/xml`)
    try:
        doxygen_xml_output_dir = (
            Path(os.environ["MIR_BUILD_DIR"]) / "doc/doxygen_output/xml"
        )
        assert doxygen_xml_output_dir.is_dir(), "Doxygen XML output not found"

        extensions.extend(["exhale", "breathe"])

        # Setup the exhale extension
        exhale_args = {
            # These arguments are required
            "containmentFolder": "./api",
            "rootFileName": "EXCLUDE",
            "doxygenStripFromPath": "..",
            # Heavily encouraged optional argument (see docs)
            "rootFileTitle": "Mir API",
            # Suggested optional arguments
            "createTreeView": False,
            "exhaleExecutesDoxygen": False,
            "exhaleUseDoxyfile": False,
            "contentsDirectives": False,
        }

        # Setup the breathe extension
        breathe_projects = {"Mir": str(doxygen_xml_output_dir)}
        breathe_default_project = "Mir"
        breathe_default_members = ("members", "undoc-members")
        breathe_order_parameters_first = True

    except (KeyError, AssertionError) as ex:
        raise Exception(
            "MIR_CMAKE_API_DOCS is set, but couldn't find Doxygen output. "
            "Make sure `MIR_BUILD_DIR` points at the CMake build directory "
            "and that `doxygen_output` is populated inside\n"
            f"{ex}"
        ) from ex


# Mermaid
mermaid_version = "10.5.0"
mermaid_d3_zoom = True

# MyST
myst_heading_anchors = 3
