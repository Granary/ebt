EBT -- **E**vent-directed dynamic **B**inary **T**ranslation
============================================================

The EBT project was started by Serhei Makarov over the summer of 2014 as part of a USRA internship with UofT's computer systems group.

The eventual aim is to build a high-level event-based language that generates client modules for a Dynamic Binary Translation framework with a minimal amount of hassle, both for passively observing a target program and for altering its execution with additional safety checks (as done, e.g. by Valgrind's `memcheck` utility). A high-level language for this purpose has a lot of potential, as writing a DBT client manually requires handling the target program code at a very low level, whereas using a pre-existing DBT client often involves giving up customizability.

The overall design mimics languages such as [SystemTap](https://sourceware.org/systemtap/) or [DTrace](http://dtrace.org/blogs/), but with additional features taking advantage of the capabilities of a DBT system such as [Granary](https://github.com/Granary/granary) or [DynamoRIO](http://www.dynamorio.org/).

You may also be interested in the [references](doc/refs.md) for my poster at PACT 2014.

setup requirements
==================

The current version of `ebt` requires `openssl-devel` (on Fedora, or equivalent library on other systems) for directory hashing support. <!-- TODOXXX we might want to use something else -->
