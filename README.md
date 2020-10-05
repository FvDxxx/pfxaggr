# pfxaggr
Yet another IP aggregation tool (fast)

Description
-----------

pfxaggr reads from STDIN prefixes in CIDR notation (v4 & v6) and outputs them sorted / aggregated / in another notation to STDOUT. 


Installation
------------
make


Usage information
-----------------

What do the different modes do? Let's take as example the following input:
```
192.0.2.0/32
192.0.2.1/32
192.0.2.0/31
192.0.2.2/31
192.0.2.7/31 (unaligned)
192.0.2.2/32
192.0.2.3/32
192.0.2.0/30
192.0.2.4/31
```

`-na` just aligns and sorts the prefixes:
```
192.0.2.0/30
192.0.2.0/31
192.0.2.0/32
192.0.2.1/32
192.0.2.2/31
192.0.2.2/32
192.0.2.3/32
192.0.2.4/31
192.0.2.6/31
```

`-a0` aggregates them by "hiding" prefixes, which are covered by less specifics seen in the input. No new less specifics are added:
```
192.0.2.0/30
192.0.2.4/31
192.0.2.6/31
```

`-a1` aggregates them but eventually adds new less specifics. The covered address space remains the same, but added less specifics might not had been seen in the input:
```
192.0.2.0/29
```

`-a2` combines the prefixes of the input in a slightly different notation for exact match with the input. prefix/mask-maxmask states, that in the input, every possible combination of prefix/m with m between mask and maxmask was present:
```
192.0.2.0/30-32
192.0.2.4/31
192.0.2.6/31
```

`-a3` combines the prefixes of the input in a slightly different notation for exact match with the input. In addition to -a2 also combines complete "sets" from the closest less-specific prefix/mask's perspective (which wasn't part of the input). prefix/mask,/mask1-mask2 states, all possible prefix/m with m between mask1 and mask2 had been present in the input. 
```
192.0.2.0/29,/31-31
192.0.2.0/30
192.0.2.0/30,/32-32
```

`-a4` is suitable as input for Juniper's `route-filter(-list)` with `upto` (prefix/mask1-mask2 => `route-filter prefix/mask1 upto /mask2`) or `prefix-length-range` (prefix/mask,/mask1-mask2 => `route-filter prefix/mask prefix-length-range /mask1-/mask2`) to exactly match input prefixes. In some cases the result is not optimal or other "solutions" are possible.
```
192.0.2.0/30-32
192.0.2.4/30,/31-31
```

PS
--

PS1: And do not use this in your data structure lessons ... there are better data structures for prefixes (radix tree / trie) than an unbalanced tree with all intermediate nodes created ... quick hack and so "easy" to understand aggregation, if you draw it on a piece of paper ... and still significant faster than most other aggregate tools.

PS2: Oh, yes, gcc knows 128bit integers, which would allow easier code writing ... but other compilers like Sun Studio don't ...

PS3: This is somewhat old code ;-)

Bugs
----

Please report bugs at https://github.com/FvDxxx/pfxaggr/issues

Author
------

Markus Weber <fvd-github@uucp.de>
