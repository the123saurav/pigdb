### Storage Engine


#### Heap File

- Created with page size and initial num pages.
- Consists of header of fixed length and actual data pages afterwards.
- The header has 
  `{pageSize, numPages, freeSpaceMapLess33, freeSpaceMapLess66, freeSpaceMapLess100}`
- The page is:

```
[Header | SlotArray | TupleBytes]

Header = {NumSlots, WriteLsn}

SlotArray is 2 bytes(allows 16KB pages) and stores offsets.
An empty slot array stores 0.
Note that a slot array can have holes in future.

Tuple = {32 bit checksum, 32 bit attr1, 32 bit attr2, 32 bit xmin, 32 bit xmax}
Since we only support fixed size int, we just need to make sure all tuples are having right alignment and do padding.
For 4 bit ints, every tuple should start at 4byte boundary.

Since there are no updates or deletes, there is no
need to store pointer(page, slot) for previous versions of the
tuple right now.
```

- The header is mmaped and mlocked at time of heap file creation.

- Note that the header(md) at all levels need to be consistent
  with data and each entity should be self contained(e.g tuple/page/heap). The only challenge it poses is
  how to keep free spacemap bounded.
  It is stored as a bitmap for compactness with each being set indicates the pageId belongs to that. A 4Kb bitmap could store 32000 pages.
  We store the spacemap in multiple places of 4KB,
  at say offset [X, X+ 4kb], [X + 32000 * page_size ]
  However initially we would have only 1 such zone.
  When a page needs to move from one zone to other, we unset the bit in 1 and set in other and do msync for now
  do atmost 2 pageIO.

  Note: future is to `use WAL` for this.

- Note there is no support for double write buffer right now.

### Index File

The index is a B+ Tree with node size == page size.
There are 2 type of nodes:
1. Internal Node: This is the non-leaf node of BTree. It would have [{less_than_key, page_id}]
2. Leaf Node: Stores [{key, (pageId, slot)}] and page_id of prev and next leaf page.

The branching factor of internal nodes is fn(page_size, header_size, sizeof(page_id), sizeof(max_key_size)).

Since only INSERT is supported, bTree would not have merge, only splits. 
It would support seq index scan.

The index does not has duplicate keys.

#### Concurrency
Since PigDb allows single writer and multiple readers with Snapshot Isolation,
we need to guarantee that splits/inserts should not leave the index in in-consistent state.
Furthermore the locking should allow for parallelism in different parts of BTree.

There would be 2 form of locking strategy:
1. Lock Crabbing for top to bottom traversal: Take locks in one direction, locking before releasing it.
   For split optimization, we have 2 options:
   a. Acq lock on child, check if insert would lead to split, if yes continue holding lock,if no release the lock 
      chain upto this node.
   b. Assume optimistically no need for a lock and go to leaf, if split happens, restart from top taking lock.

2. For leaf level traversal(f/w or b/w), try to acq lock on next node, if can't acquire, give up and retry from root again.
   This is akin to Deadlock Prevention at the cost of throwaway work and starvation. 
   This is where a `LatchManager` may be helpful to help probe who is owning the lock, its priority and so on.
   Another approach is to retry with an upgraded lock from root.


