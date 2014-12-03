# -*- coding: utf-8 -*-
#
# -- General configuration -----------------------------------------------------

source_suffix = '.rst'

# Name of the master file 
master_doc = 'index'

# General information about the project.
project = u'x265'

# This is the Copyright Information that will appear on the bottom of the document
copyright = u'2014 MulticoreWare Inc'

# -- Options for HTML output ---------------------------------------------------
html_theme = "default"

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
    ('index', 'libx265', 'Full x265 Documentation',
    ['MulticoreWare Inc'], 3),
    ('x265', 'x265', 'x265 CLI Documentation',
    ['MulticoreWare Inc'], 1)
]
