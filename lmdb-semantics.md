# LMDB Semantics
LMDB is very powerful and fast and implements a simplified variant of the
BDB-api. This too is very powerful, and verbosely documented. In this file,
I hope to offer the very basic things you need to know. I also want to thank
Howard Chu for valuable feedback, and for maintaining LMDB of course!

This document does not aim to be complete, but it does strive to be very clear
and direct. Afterwards, the [API documentation](http://symas.com/mdb/doc/group__mdb.html#details)
should make sense.

It all starts with an environment, as generated with **mdb\_env\_create()**.
Once created, this environment must also be opened with **mdb\_env\_open()**.

**mdb\_env\_open()** gets passed a name which is interpreted as a directory
(which must exist already!).  Within that directory, a lock file and a
storage file will be generated.  If you don't want that, pass MDB\_NOSUBDIR,
and instead a file plus another file with a "-lock" extension will be used.

Now that the environment is open, we can generate a transaction within it
using **mdb\_txn\_begin()**.  Transactions can be nested, but need not be. 
Note that a transaction must only be used by 1 thread at a time!  We always
need a transaction, even for read-only access. The transaction is our consistent
view on the data.

Now that we have a transaction, within it we can open a database using
**mdb\_dbi\_open()**. If we only want one database, we can pass the value 0
as its name.  For named databases, pass MDB\_CREATE the first time it is
opened.  Also rember to call **mdb\_env\_set\_maxdbs()** after
**mdb\_env\_create()** and before **mdb\_env\_open()** to set the maximum
number of named datbases you want to support.

Note: a single transaction can open multiple databases.

In a transaction, **mdb\_get()** and **mdb\_put()** can store single key/value pairs
if that is all you need to do (but see Cursors below if you want to do more).

A key/value pair is expressed as two **MDB\_val** structures. This struct
has two attributes, mv\_size and mv\_data. The data is a void pointer to an
array of mv\_size bytes. 

Because LMDB is very efficient, the data returned in an **MDB\_val** structure
may be memory mapped straight from disk. In other words, **look but do not
touch** (or free() for that matter). Once a transaction is closed, the
values can no longer be used. So make a copy if you need to keep them
around!

## Cursors
To do more powerful things, we need a cursor.

Within the transaction, we can generate a cursor with **mdb\_cursor\_open()**.  And
with this cursor we can store/retrieve/delete (multiple) values using
**mdb\_cursor\_get()**, **mdb\_cursor\_put()** and **mdb\_cursor\_del()**.

**mdb\_cursor\_get()** positions itself depending on the cursor operation
requested, and for some operations, on the supplied key. To list all
key/value pairs in a database for example, use operation MDB\_FIRST for the
first call to **mdb\_cursor\_get()**, and MDB\_NEXT on subsequent calls,
until the end is hit.

To retrieve all keys starting from a specified key value, use
MDB\_SET\_RANGE.  For more cursor operations, see the [API
docs](http://symas.com/mdb/doc/group__mdb.html).

When using **mdb\_cursor\_put()**, either the function will position the
cursor for you based on the **key**, or you can use operation MDB\_CURRENT 
to use the current position of the cursor. Note that **key** must then match
that current position's key!

# Summarizing the opening

So we have a cursor in a transaction which opened a database in an
environment which is opened from a filesystem after it was separately created.

Or, we create an environment, open it from a filesystem, create a
transaction within it, open datbase within that and create a cursor within
all of the above.

Easy, right? 

# Transactions, rollbacks etc
As expected, to get anything actually done, a transaction must be committed
using **mdb\_txn\_commit()**, or aborted using **mdb\_txn\_abort()**.  If
this was a read-only transaction, any cursors will NOT be freed.  If it was
one that could write (ie, created without MDB_RDONLY), all cursors will be
freed, and should not be used anymore.

For read-only transactions, there is no need to commit anything to storage
of course. The LMDB transaction however should still eventually be
"committed" or aborted to close the database handle(s) opened in them.

In addition, as long as a transaction is open, a consistent view of the database
is kept alive, which requires storage. A read-only transaction that no
longer requires this consistent view should therefore be aborted (but see
below for an optimization).

There can be multiple simultaneously active read-only transactions (ie, they
were opened as read-only), but only one that could write. Once a single write-capable
transaction is opened, all further attempts to begin one will block until the first
one is committed or aborted. It is still possible to begin read-only views
however.

# Duplicate keys
**mdb\_get()** and **mdb\_put()** respectively have no and only some support for
multiple key/value pairs with identical keys. If there are multiple values
for one key, **mdb\_get()** will only return the 'first' one.

When multiple values for one key are required, pass the MDB\_DUPSORT flag to
**mdb\_dbi\_open()**. In an MDB\_DUPSORT database, **mdb\_put()** (by
default) will not replace the value for a key if it existed already, but add
the new value to it.  In addition, **mdb\_del()** starts paying attention to
the value field too, allowing for precision deletion.

Finally, further cursor operations become available for traversing through
and retrieving duplicate values.

# Some optimization
If you frequently begin and abort read-only transactions, as an optimization,
it is possible to only reset and renew a transaction. 

**mdb\_txn\_reset()** releases any old copies of data kept around for a
read-only transaction.  To revive this reset transaction, call
**mdb\_txn\_renew()** on it.  If this is done, and cursors were in use,
these too must be renewed using **mdb\_cursor\_renew()**.

To permanently free a transaction, reset or not, call
**mdb\_txn\_abort()** on it.

# Cleaning up
It is important to close transactions in order to make database handles
available again.

If a transaction was not write-capable, any cursors created within it should
be destroyed manually.

It is very rarely necessary to close a database handle, since it gets closed
with the transaction.

# Now read up on the full API!

The [full API](http://symas.com/mdb/doc/group__mdb.html#details) lists
further details, like how to:

  * size a database (the default limits are rather low)
  * drop and clean a database
  * detect and report errors 
  * optimize (bulk) loading speed
  * (temporarily) reduce robustness to gain even more speed
  * gather statistics on your database
  * define custom sort orders
  

