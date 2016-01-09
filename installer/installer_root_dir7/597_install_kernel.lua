
return {
    id = "install_kernel",
    name = _("Install Kernel"),
    effect = function(step)
	
	local cmds = CmdChain.new()
	cmds:add("tar xzpf /kernels/kernel_SMP.gz -C /mnt/boot/")
	cmds:add("echo SMP > /mnt/boot/kernel/pfsense_kernel.txt")
	cmds:execute()

	return step:next()
    end
}
