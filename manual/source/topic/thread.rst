.. sources:

    `<https://info.ravenbrook.com/project/mps/master/design/thread-safety/>`_

.. index::
   single: thread

.. _topic-thread:

Threads
=======

.. index::
   single: thread safety

Thread safety
-------------

The MPS is designed to run in an environment with multiple threads all
calling into the MPS. Some code is known to operate with exclusive
access to the data it manipulates (for example, allocation via
:term:`allocation points`, in the common case where the buffer does
not need to be refilled, and :term:`location dependencies`), so this
code is safe. For the rest of the code, shared data structures are
locked by the use of a single lock per :term:`arena`. This lock is
claimed on entry to the MPS and released on exit from it. So there is
at most a single thread (per arena) running "inside" the MPS at a
time.


.. index::
   single: thread; registration

.. _topic-thread-register:

Thread registration
-------------------

In order to scan a thread's registers for references (which happens at
each :term:`flip`), the MPS needs to be able to suspend that thread,
and in order to gain exclusive atomic access to memory in order to
scan it, the MPS needs to be able to suspend all threads that might
access that memory. This means that threads must be registered with
the MPS by calling :c:func:`mps_thread_reg` (and thread roots created;
see :ref:`topic-root-thread`).

For simplicity, we recommend that a thread must be registered with an
:term:`arena` if:

* its registers and control stack form a root (this is enforced by
  :c:func:`mps_root_create_reg`); or

* it reads or writes from a location in an :term:`automatically managed
  <automatic memory management>` :term:`pool` in the arena.

However, some automatically managed pool classes may be more liberal
than this. See the documentation for the pool class.


.. index::
   single: signal; handling
   single: exception; handling
   single: thread; signal handling
   single: thread; exception handling

.. _topic-thread-signal:

Signal and exception handling issues
------------------------------------

.. warning::

    On Unix platforms (except OS X), the MPS suspends and resumes
    threads by sending them signals. There's a shortage of available
    signals that aren't already dedicated to other purposes (for
    example, ValGrind uses ``SIGUSR1`` and ``SIGUSR2``), so the MPS uses
    ``SIGXCPU`` and ``SIGXFSZ``. This means that programs must not mask
    these two signals.

    If your program needs to handle these signals, then it must
    co-operate with the MPS. At present, there's no documented
    mechanism for co-operating: if you are in this situation, please
    :ref:`contact us <contact>`.

.. warning::

    The MPS uses :term:`barriers (1)` to :term:`protect <protection>`
    memory from the :term:`client program` and handles the signals that
    result from barrier hits.

    * On Linux and FreeBSD, your program must not mask or handle ``SIGSEGV``.
    
    * On Windows, you must not install a first-chance exception handler.
    
    * On OS X, you must not install a thread-local Mach exception handler
      for ``EXC_BAD_ACCESS`` exceptions.

    All of these things are, in fact, possible, but your program must
    co-operate with the MPS. At present, there's no documented mechanism
    for co-operating: if you are in this situation, please :ref:`contact
    us <contact>`.

.. index::
   single: thread; interface

Thread interface
----------------

.. c:type:: mps_thr_t

    The type of registered :term:`thread` descriptions.

    In a multi-threaded environment where :term:`incremental garbage
    collection` is used, threads must be registered with the MPS by
    calling :c:func:`mps_thread_reg` so that the MPS can suspend them
    as necessary in order to have exclusive access to their state.

    Even in a single-threaded environment it may be necessary to
    register a thread with the MPS so that its stack can be registered
    as a :term:`root` by calling :c:func:`mps_root_create_reg`.


.. c:function:: mps_res_t mps_thread_reg(mps_thr_t *thr_o, mps_arena_t arena)

    Register the current :term:`thread` with an :term:`arena`.

    ``thr_o`` points to a location that will hold the address of the
    registered thread description, if successful.

    ``arena`` is the arena.

    Returns :c:macro:`MPS_RES_OK` if successful, or another
    :term:`result code` if not.

    A thread must be registered with an arena if it ever uses a
    pointer to a location in an :term:`automatically managed
    <automatic memory management>` :term:`pool` belonging to that
    arena.

    .. note::

        It is recommended that all threads be registered with all
        arenas.


.. c:function:: void mps_thread_dereg(mps_thr_t thr)

    Deregister a :term:`thread`.

    ``thr`` is the description of the thread.

    After calling this function, the thread whose registration with an
    :term:`arena` was recorded in ``thr`` must not read or write from
    a location in an :term:`automatically managed <automatic memory
    management>` :term:`pool` belonging to that arena.

    .. note::

        Some pool classes may be more liberal about what a thread may
        do after it has been deregistered. See the documentation for
        the pool class.

    .. note::

        It is recommended that threads be deregistered only when they
        are just about to exit.
