`shared_mutex` that can
=======================

Overview
--------
A super-quick non-locking shareable mutex with ability to promote a lock from shared mode to exclusive mode and back.
Written in `C++` with `<atomic>` library including memory ordering tweaking to achieve what is missing in both `std` and `boost` `shared_mutex` / `shared_lock`.

You can create `shared_lock` or `exclusive_lock` and then upgrade / downgrade it with `upgrade_to_exclusive()` and `downgrade_to_shared()` methods.

Basically, it implements multiple readers / multiple writers concept where any of the threads can be promoted to writer (and go back to reader) at any time.

Also, there are three waiting strategies: `burn` - cycles the cpu, `wait` - sleeps for some given time, `yield` - switches to another thread.

See the tests for examples. 

```c++
	gx::shared_mutex m;
	// ...
	// thread1
	{
		gx::shared_lock l(m);
		// do shareable work
		l.upgrade_to_exclusive();
		// do exclusive work
		l.downgrade_to_shared();
		// do shareable work
	}
	// ...
	// thread2
	{
		gx::exclusive_lock l(m);
		// do exclusive work
		l.downgrade_to_shared();
		// do shareable work
		l.upgrade_to_exclusive();
		// do exclusive work
	}
	// ...
	// thread3
	{
		gx::shareable_lock l(m, gx::shared);
		// do shareable work
		l.upgrade_to_exclusive();
		// do exclusive work
	}
```
