For Tomahawk 2 and 3, you need a cross-compiler for Xtensa. M3 uses buildroot for that. Just clone
the repository to some location (the folder of this README file in this example; note that you have
to copy the overlay directory appropriately, if you clone to repository to somewhere else):

    $ git clone git://git.buildroot.net/buildroot

Now checkout the 2016.02 branch

    $ cd buildroot
    $ git checkout -b m3 2016.02

Finally, copy the config file and build it:

    $ cp ../config-buildroot-iss .config
    $ make -j
