exec_process
============

exec_process unifies frequently needed functionality to interact with child
processes in a convenient function.

It gives you the possibility to spawn your children in a blocking fashion,
where execution of the main program is suspended until the child exists.

It gives you the possibility to spawn your children in a non-blocking fashion,
where execution returns to the parent as soon as the child process has been
created.

Addtitionally, it provides a very basic interface to extract all information
needed in order to determine what has happened in the parent, child or in the
newly executed process image.
