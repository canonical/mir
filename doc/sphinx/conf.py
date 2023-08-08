import sys

sys.path.append('./')
from custom_conf import *

# Configuration file for the Sphinx documentation builder.
# You should not do any modifications to this file. Put your custom
# configuration into the custom_conf.py file.
# If you need to change this file, contribute the changes upstream.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

############################################################
### Extensions
############################################################

extensions = [
    'sphinx_design',
    'sphinx_tabs.tabs',
    'sphinx_reredirects',
    'youtube-links',
    'related-links',
    'custom-rst-roles',
    'terminal-output',
    'sphinx_copybutton',
    'sphinxext.opengraph',
    'myst_parser',
    'sphinxcontrib.jquery',
    'notfound.extension'
]
extensions.extend(custom_extensions)

### Configuration for extensions

# Additional MyST syntax
myst_enable_extensions = [
    'substitution',
    'deflist'
]

# Used for related links
if 'discourse' in html_context:
    html_context['discourse_prefix'] = html_context['discourse'] + '/t/'

# The default for notfound_urls_prefix usually works, but not for
# documentation on documentation.ubuntu.com
if slug:
    notfound_urls_prefix = '/' + slug + '/en/latest/'

notfound_context = {
    'title': 'Page not found',
    'body': '<h1>Page not found</h1>\n\n<p>Sorry, but the documentation page that you are looking for was not found.</p>\n<p>Documentation changes over time, and pages are moved around. We try to redirect you to the updated content where possible, but unfortunately, that didn\'t work this time (maybe because the content you were looking for does not exist in this version of the documentation).</p>\n<p>You can try to use the navigation to locate the content you\'re looking for, or search for a similar page.</p>\n',
}

# Default image for OGP (to prevent font errors, see
# https://github.com/canonical/sphinx-docs-starter-pack/pull/54 )
if not 'ogp_image' in locals():
    ogp_image = 'https://assets.ubuntu.com/v1/253da317-image-document-ubuntudocs.svg'

############################################################
### General configuration
############################################################

exclude_patterns = [
    '_build',
    'Thumbs.db',
    '.DS_Store',
    '.sphinx',
    'doc-cheat-sheet*',
]
exclude_patterns.extend(custom_excludes)

rst_epilog = '''
.. include:: /reuse/links.txt
'''
if 'custom_rst_epilog' in locals():
    rst_epilog = custom_rst_epilog

source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}

if not 'conf_py_path' in html_context and 'github_folder' in html_context:
    html_context['conf_py_path'] = html_context['github_folder']

############################################################
### Styling
############################################################

# Find the current builder
builder = 'dirhtml'
if '-b' in sys.argv:
    builder = sys.argv[sys.argv.index('-b')+1]

# Setting templates_path for epub makes the build fail
if builder == 'dirhtml' or builder == 'html':
    templates_path = ['.sphinx/_templates']

# Theme configuration
html_theme = 'furo'
html_last_updated_fmt = ''
html_permalinks_icon = '¶'
html_theme_options = {
    'light_css_variables': {
        'font-stack': 'Ubuntu, -apple-system, Segoe UI, Roboto, Oxygen, Cantarell, Fira Sans, Droid Sans, Helvetica Neue, sans-serif',
        'font-stack--monospace': 'Ubuntu Mono, Consolas, Monaco, Courier, monospace',
        'color-foreground-primary': '#111',
        'color-foreground-secondary': 'var(--color-foreground-primary)',
        'color-foreground-muted': '#333',
        'color-background-secondary': '#FFF',
        'color-background-hover': '#f2f2f2',
        'color-brand-primary': '#111',
        'color-brand-content': '#06C',
        'color-api-background': '#cdcdcd',
        'color-inline-code-background': 'rgba(0,0,0,.03)',
        'color-sidebar-link-text': '#111',
        'color-sidebar-item-background--current': '#ebebeb',
        'color-sidebar-item-background--hover': '#f2f2f2',
        'toc-font-size': 'var(--font-size--small)',
        'color-admonition-title-background--note': 'var(--color-background-primary)',
        'color-admonition-title-background--tip': 'var(--color-background-primary)',
        'color-admonition-title-background--important': 'var(--color-background-primary)',
        'color-admonition-title-background--caution': 'var(--color-background-primary)',
        'color-admonition-title--note': '#24598F',
        'color-admonition-title--tip': '#24598F',
        'color-admonition-title--important': '#C7162B',
        'color-admonition-title--caution': '#F99B11',
        'color-highlighted-background': '#EbEbEb',
        'color-link-underline': 'var(--color-background-primary)',
        'color-link-underline--hover': 'var(--color-background-primary)',
        'color-version-popup': '#772953'
    },
    'dark_css_variables': {
        'color-foreground-secondary': 'var(--color-foreground-primary)',
        'color-foreground-muted': '#CDCDCD',
        'color-background-secondary': 'var(--color-background-primary)',
        'color-background-hover': '#666',
        'color-brand-primary': '#fff',
        'color-brand-content': '#06C',
        'color-sidebar-link-text': '#f7f7f7',
        'color-sidebar-item-background--current': '#666',
        'color-sidebar-item-background--hover': '#333',
        'color-admonition-background': 'transparent',
        'color-admonition-title-background--note': 'var(--color-background-primary)',
        'color-admonition-title-background--tip': 'var(--color-background-primary)',
        'color-admonition-title-background--important': 'var(--color-background-primary)',
        'color-admonition-title-background--caution': 'var(--color-background-primary)',
        'color-admonition-title--note': '#24598F',
        'color-admonition-title--tip': '#24598F',
        'color-admonition-title--important': '#C7162B',
        'color-admonition-title--caution': '#F99B11',
        'color-highlighted-background': '#666',
        'color-link-underline': 'var(--color-background-primary)',
        'color-link-underline--hover': 'var(--color-background-primary)',
        'color-version-popup': '#F29879'
    },
}

############################################################
### Additional files
############################################################

html_static_path = ['.sphinx/_static']

html_css_files = [
    'custom.css',
    'header.css',
    'github_issue_links.css',
]
html_css_files.extend(custom_html_css_files)

html_js_files = ['header-nav.js']
if 'github_issues' in html_context and html_context['github_issues'] and not disable_feedback_button:
    html_js_files.append('github_issue_links.js')
html_js_files.extend(custom_html_js_files)
