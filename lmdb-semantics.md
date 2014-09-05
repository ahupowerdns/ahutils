# LMDB Semantics
LMDB is very powerful and fast and implements the BDB-api. This too is very
powerful, and verbosely documented. In this file, I hope to offer the very
basic things you need to know. The API itself is documented 
[here](http://symas.com/mdb/doc/group__mdb.html#details).

It all starts with an environment, as generated with **mdb\_env\_create()**.
Once created, this environment must also be opened with **mdb\_env\_open()**.

**mdb\_env\_open()** gets passed a name which is interpreted as a directory, and
this directory will be created for you if it does not exist yet. Within that
directory, a lock file and a storage file will be generated. If you don't
want that, pass MDB\_NOSUBDIR, and a file plus a file with a "-lock"
extension will be used. 

Now that the environment is open, we can generate a transaction within it
using **mdb\_txn\_begin()**.  Transactions can be nested, but need not be. 
Note that a transaction must only be used by 1 thread at a time!  We always
need a transaction, even for read-only access.

Now that we have a transaction, within it we can open a database using
**mdb\_dbi\_open()**. If we only want one database, we can pass the value 0
as its name.  For named databases, pass MDB\_CREATE the first time it is
opened.  Also rember to call **mdb\_env\_set\_maxdbs()** after
**mdb\_env\_create()** and before **mdb\_env\_open()** to set the maximum
number of named datbases you want to support.

Note: a single transaction can open multiple databases.

In this transaction, **mdp\_get()** and **mdp\_put()** can store single key/value pairs
if that is what you need to do. 

A key/value pair is expressed as a two **MDP\_val** structures. This struct
has two attributes, mv\_size and mv\_data. The data is a void pointer to an
array of mv\_size bytes. 

Because LMDB is very efficient, the data returned in an **MDP\_val** structure
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
requested, and for some operations, on the supplied key. To list all keys in
a database for example, use operation MDB\_FIRST for the first call to
**mdb\_cursor\_get()**, and MDB\_NEXT on subsequent calls, until the end is
hit. 

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
one that could write, all cursors will be freed, and should not be used
anymore.

For read-only transactions, there is no need to commit them of course.
However, as long as a transaction is open, a consistent view of the database
is kept alive, which requires storage. A read-only transaction that no
longer requires this consistent view should therefore be reset using
**mdb\_txn\_reset()**, which releases any old copies of data kept around.

To revive this reset transaction, call **mdb\_txn\_renew()** on it.
If this is done, and cursors were in use, these too must be renewed using
**mdb\_cursor\_renew()**.

Alternatively to free a transaction which has been reset, call
**mdb\_txn\_abort()** on it too.

