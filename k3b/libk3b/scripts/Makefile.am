bin_SCRIPTS = k3b_automount

# k3b_automount needs to be installed setuid root
install-exec-hook:
	@(chown 0 $(DESTDIR)$(bindir)/k3b_automount && chmod 4755 $(DESTDIR)$(bindir)/k3b_automount) || echo "Please make k3b_automount setuid root" >&2
	@echo ""
	@echo "k3b_automount is by default installed with a set SETUID root bit!"
	@echo "This is needed for K3b to be able to temporarily disable automounting via"
	@echo "subfs or supermount while burning."
	@echo ""

