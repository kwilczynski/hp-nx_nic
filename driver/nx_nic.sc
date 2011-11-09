
'''
Driver definition for nx_nic driver.
'''

defineVmkDriver(
   name='nx_nic',
   version='0.0.0',
   description='NetXen 10G Ethernet Driver',
   driverType='net',
   files=[('drivers/net/nx_nic',
           Split('''
		unm_nic_hw.c 
           unm_nic_main.c 
           unm_nic_init.c 
           unm_nic_ethtool.c 
           unm_nic_lro.c 
           unm_nic_procfs.c 
           unm_nic_snmp.c 
           unm_nic_tool.c 
           nx_nic_vmk.c 
	   nxhal.c
	   nxhal_v34.c
           nx_mem_pool.c 
           nx_hash_table.c 
           xge_mdio.c 
           niu.c 
	   nx_pexq.c
                 '''))],
   appends=dict(CPPDEFINES={
                'PEGNET_NIC':None,
                'UNM_CONF_PROCESSOR':'UNM_CONF_X86',
                'UNM_CONF_OS':'UNM_CONF_LINUX',
                'UNM_HAL_NATIVE':None,
                'UNM_X_HARDWARE':'UNM_X_ASIC',
                },
                CCFLAGS=['-Werror', '-I/opt/vmware/ddk/src/vmkdrivers/src26/drivers/net/nx_nic/include'],
               ),
   heapinfo=('8*1024*1024', '32*1024*1024'),
)

