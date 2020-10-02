# pfxaggr
Yet another IP aggregation tool

Just quickly uploaded this old piece of code ... should write some info (on -a0, -a1), potential issues and optimization potential on -a2, -a3 and -a4) on day ... and for sure clean up the code ... but not now.

One thing I just found (after all the years ;-): IPv4-mapped addresses ... ::ffff:0.0.0.0/128 and ::ffff:0.0.0.1/128 will be aggregated and reported as ::ffff:0.0.0.0/127 ... not really what one would expect.

Oh, an issue ... outing myself as not-too-familiar with Github ...

Anyway, have fun. 

And do not use this in your data structure lessons ... there are better data structures (radix tree / trie) than a tree with all intermediate nodes created.
