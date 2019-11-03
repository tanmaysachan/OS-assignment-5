1550 #include "param.h"
1551 #include "types.h"
1552 #include "defs.h"
1553 #include "x86.h"
1554 #include "memlayout.h"
1555 #include "mmu.h"
1556 #include "proc.h"
1557 #include "elf.h"
1558 
1559 extern char data[];  // defined by kernel.ld
1560 pde_t *kpgdir;  // for use in scheduler()
1561 
1562 // Set up CPU's kernel segment descriptors.
1563 // Run once on entry on each CPU.
1564 void
1565 seginit(void)
1566 {
1567   struct cpu *c;
1568 
1569   // Map "logical" addresses to virtual addresses using identity map.
1570   // Cannot share a CODE descriptor for both kernel and user
1571   // because it would have to have DPL_USR, but the CPU forbids
1572   // an interrupt from CPL=0 to DPL=3.
1573   c = &cpus[cpuid()];
1574   c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
1575   c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
1576   c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
1577   c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
1578   lgdt(c->gdt, sizeof(c->gdt));
1579 }
1580 
1581 // Return the address of the PTE in page table pgdir
1582 // that corresponds to virtual address va.  If alloc!=0,
1583 // create any required page table pages.
1584 static pte_t *
1585 walkpgdir(pde_t *pgdir, const void *va, int alloc)
1586 {
1587   pde_t *pde;
1588   pte_t *pgtab;
1589 
1590   pde = &pgdir[PDX(va)];
1591   if(*pde & PTE_P){
1592     pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
1593   } else {
1594     if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
1595       return 0;
1596     // Make sure all those PTE_P bits are zero.
1597     memset(pgtab, 0, PGSIZE);
1598     // The permissions here are overly generous, but they can
1599     // be further restricted by the permissions in the page table
1600     // entries, if necessary.
1601     *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
1602   }
1603   return &pgtab[PTX(va)];
1604 }
1605 
1606 // Create PTEs for virtual addresses starting at va that refer to
1607 // physical addresses starting at pa. va and size might not
1608 // be page-aligned.
1609 static int
1610 mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
1611 {
1612   char *a, *last;
1613   pte_t *pte;
1614 
1615   a = (char*)PGROUNDDOWN((uint)va);
1616   last = (char*)PGROUNDDOWN(((uint)va) + size - 1);
1617   for(;;){
1618     if((pte = walkpgdir(pgdir, a, 1)) == 0)
1619       return -1;
1620     if(*pte & PTE_P)
1621       panic("remap");
1622     *pte = pa | perm | PTE_P;
1623     if(a == last)
1624       break;
1625     a += PGSIZE;
1626     pa += PGSIZE;
1627   }
1628   return 0;
1629 }
1630 
1631 // There is one page table per process, plus one that's used when
1632 // a CPU is not running any process (kpgdir). The kernel uses the
1633 // current process's page table during system calls and interrupts;
1634 // page protection bits prevent user code from using the kernel's
1635 // mappings.
1636 //
1637 // setupkvm() and exec() set up every page table like this:
1638 //
1639 //   0..KERNBASE: user memory (text+data+stack+heap), mapped to
1640 //                phys memory allocated by the kernel
1641 //   KERNBASE..KERNBASE+EXTMEM: mapped to 0..EXTMEM (for I/O space)
1642 //   KERNBASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
1643 //                for the kernel's instructions and r/o data
1644 //   data..KERNBASE+PHYSTOP: mapped to V2P(data)..PHYSTOP,
1645 //                                  rw data + free physical memory
1646 //   0xfe000000..0: mapped direct (devices such as ioapic)
1647 //
1648 // The kernel allocates physical memory for its heap and for user memory
1649 // between V2P(end) and the end of physical memory (PHYSTOP)
1650 // (directly addressable from end..P2V(PHYSTOP)).
1651 
1652 // This table defines the kernel's mappings, which are present in
1653 // every process's page table.
1654 static struct kmap {
1655   void *virt;
1656   uint phys_start;
1657   uint phys_end;
1658   int perm;
1659 } kmap[] = {
1660  { (void*)KERNBASE, 0,             EXTMEM,    PTE_W}, // I/O space
1661  { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},     // kern text+rodata
1662  { (void*)data,     V2P(data),     PHYSTOP,   PTE_W}, // kern data+memory
1663  { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W}, // more devices
1664 };
1665 
1666 // Set up kernel part of a page table.
1667 pde_t*
1668 setupkvm(void)
1669 {
1670   pde_t *pgdir;
1671   struct kmap *k;
1672 
1673   if((pgdir = (pde_t*)kalloc()) == 0)
1674     return 0;
1675   memset(pgdir, 0, PGSIZE);
1676   if (P2V(PHYSTOP) > (void*)DEVSPACE)
1677     panic("PHYSTOP too high");
1678   for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
1679     if(mappages(pgdir, k->virt, k->phys_end - k->phys_start,
1680                 (uint)k->phys_start, k->perm) < 0) {
1681       freevm(pgdir);
1682       return 0;
1683     }
1684   return pgdir;
1685 }
1686 
1687 // Allocate one page table for the machine for the kernel address
1688 // space for scheduler processes.
1689 void
1690 kvmalloc(void)
1691 {
1692   kpgdir = setupkvm();
1693   switchkvm();
1694 }
1695 
1696 
1697 
1698 
1699 
1700 // Switch h/w page table register to the kernel-only page table,
1701 // for when no process is running.
1702 void
1703 switchkvm(void)
1704 {
1705   lcr3(V2P(kpgdir));   // switch to the kernel page table
1706 }
1707 
1708 // Switch TSS and h/w page table to correspond to process p.
1709 void
1710 switchuvm(struct proc *p)
1711 {
1712   if(p == 0)
1713     panic("switchuvm: no process");
1714   if(p->kstack == 0)
1715     panic("switchuvm: no kstack");
1716   if(p->pgdir == 0)
1717     panic("switchuvm: no pgdir");
1718 
1719   pushcli();
1720   mycpu()->gdt[SEG_TSS] = SEG16(STS_T32A, &mycpu()->ts,
1721                                 sizeof(mycpu()->ts)-1, 0);
1722   mycpu()->gdt[SEG_TSS].s = 0;
1723   mycpu()->ts.ss0 = SEG_KDATA << 3;
1724   mycpu()->ts.esp0 = (uint)p->kstack + KSTACKSIZE;
1725   // setting IOPL=0 in eflags *and* iomb beyond the tss segment limit
1726   // forbids I/O instructions (e.g., inb and outb) from user space
1727   mycpu()->ts.iomb = (ushort) 0xFFFF;
1728   ltr(SEG_TSS << 3);
1729   lcr3(V2P(p->pgdir));  // switch to process's address space
1730   popcli();
1731 }
1732 
1733 // Load the initcode into address 0 of pgdir.
1734 // sz must be less than a page.
1735 void
1736 inituvm(pde_t *pgdir, char *init, uint sz)
1737 {
1738   char *mem;
1739 
1740   if(sz >= PGSIZE)
1741     panic("inituvm: more than a page");
1742   mem = kalloc();
1743   memset(mem, 0, PGSIZE);
1744   mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W|PTE_U);
1745   memmove(mem, init, sz);
1746 }
1747 
1748 
1749 
1750 // Load a program segment into pgdir.  addr must be page-aligned
1751 // and the pages from addr to addr+sz must already be mapped.
1752 int
1753 loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
1754 {
1755   uint i, pa, n;
1756   pte_t *pte;
1757 
1758   if((uint) addr % PGSIZE != 0)
1759     panic("loaduvm: addr must be page aligned");
1760   for(i = 0; i < sz; i += PGSIZE){
1761     if((pte = walkpgdir(pgdir, addr+i, 0)) == 0)
1762       panic("loaduvm: address should exist");
1763     pa = PTE_ADDR(*pte);
1764     if(sz - i < PGSIZE)
1765       n = sz - i;
1766     else
1767       n = PGSIZE;
1768     if(readi(ip, P2V(pa), offset+i, n) != n)
1769       return -1;
1770   }
1771   return 0;
1772 }
1773 
1774 // Allocate page tables and physical memory to grow process from oldsz to
1775 // newsz, which need not be page aligned.  Returns new size or 0 on error.
1776 int
1777 allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
1778 {
1779   char *mem;
1780   uint a;
1781 
1782   if(newsz >= KERNBASE)
1783     return 0;
1784   if(newsz < oldsz)
1785     return oldsz;
1786 
1787   a = PGROUNDUP(oldsz);
1788   for(; a < newsz; a += PGSIZE){
1789     mem = kalloc();
1790     if(mem == 0){
1791       cprintf("allocuvm out of memory\n");
1792       deallocuvm(pgdir, newsz, oldsz);
1793       return 0;
1794     }
1795     memset(mem, 0, PGSIZE);
1796     if(mappages(pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
1797       cprintf("allocuvm out of memory (2)\n");
1798       deallocuvm(pgdir, newsz, oldsz);
1799       kfree(mem);
1800       return 0;
1801     }
1802   }
1803   return newsz;
1804 }
1805 
1806 // Deallocate user pages to bring the process size from oldsz to
1807 // newsz.  oldsz and newsz need not be page-aligned, nor does newsz
1808 // need to be less than oldsz.  oldsz can be larger than the actual
1809 // process size.  Returns the new process size.
1810 int
1811 deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
1812 {
1813   pte_t *pte;
1814   uint a, pa;
1815 
1816   if(newsz >= oldsz)
1817     return oldsz;
1818 
1819   a = PGROUNDUP(newsz);
1820   for(; a  < oldsz; a += PGSIZE){
1821     pte = walkpgdir(pgdir, (char*)a, 0);
1822     if(!pte)
1823       a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;
1824     else if((*pte & PTE_P) != 0){
1825       pa = PTE_ADDR(*pte);
1826       if(pa == 0)
1827         panic("kfree");
1828       char *v = P2V(pa);
1829       kfree(v);
1830       *pte = 0;
1831     }
1832   }
1833   return newsz;
1834 }
1835 
1836 
1837 
1838 
1839 
1840 
1841 
1842 
1843 
1844 
1845 
1846 
1847 
1848 
1849 
1850 // Free a page table and all the physical memory pages
1851 // in the user part.
1852 void
1853 freevm(pde_t *pgdir)
1854 {
1855   uint i;
1856 
1857   if(pgdir == 0)
1858     panic("freevm: no pgdir");
1859   deallocuvm(pgdir, KERNBASE, 0);
1860   for(i = 0; i < NPDENTRIES; i++){
1861     if(pgdir[i] & PTE_P){
1862       char * v = P2V(PTE_ADDR(pgdir[i]));
1863       kfree(v);
1864     }
1865   }
1866   kfree((char*)pgdir);
1867 }
1868 
1869 // Clear PTE_U on a page. Used to create an inaccessible
1870 // page beneath the user stack.
1871 void
1872 clearpteu(pde_t *pgdir, char *uva)
1873 {
1874   pte_t *pte;
1875 
1876   pte = walkpgdir(pgdir, uva, 0);
1877   if(pte == 0)
1878     panic("clearpteu");
1879   *pte &= ~PTE_U;
1880 }
1881 
1882 // Given a parent process's page table, create a copy
1883 // of it for a child.
1884 pde_t*
1885 copyuvm(pde_t *pgdir, uint sz)
1886 {
1887   pde_t *d;
1888   pte_t *pte;
1889   uint pa, i, flags;
1890   char *mem;
1891 
1892   if((d = setupkvm()) == 0)
1893     return 0;
1894   for(i = 0; i < sz; i += PGSIZE){
1895     if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
1896       panic("copyuvm: pte should exist");
1897     if(!(*pte & PTE_P))
1898       panic("copyuvm: page not present");
1899     pa = PTE_ADDR(*pte);
1900     flags = PTE_FLAGS(*pte);
1901     if((mem = kalloc()) == 0)
1902       goto bad;
1903     memmove(mem, (char*)P2V(pa), PGSIZE);
1904     if(mappages(d, (void*)i, PGSIZE, V2P(mem), flags) < 0) {
1905       kfree(mem);
1906       goto bad;
1907     }
1908   }
1909   return d;
1910 
1911 bad:
1912   freevm(d);
1913   return 0;
1914 }
1915 
1916 // Map user virtual address to kernel address.
1917 char*
1918 uva2ka(pde_t *pgdir, char *uva)
1919 {
1920   pte_t *pte;
1921 
1922   pte = walkpgdir(pgdir, uva, 0);
1923   if((*pte & PTE_P) == 0)
1924     return 0;
1925   if((*pte & PTE_U) == 0)
1926     return 0;
1927   return (char*)P2V(PTE_ADDR(*pte));
1928 }
1929 
1930 // Copy len bytes from p to user address va in page table pgdir.
1931 // Most useful when pgdir is not the current page table.
1932 // uva2ka ensures this only works for PTE_U pages.
1933 int
1934 copyout(pde_t *pgdir, uint va, void *p, uint len)
1935 {
1936   char *buf, *pa0;
1937   uint n, va0;
1938 
1939   buf = (char*)p;
1940   while(len > 0){
1941     va0 = (uint)PGROUNDDOWN(va);
1942     pa0 = uva2ka(pgdir, (char*)va0);
1943     if(pa0 == 0)
1944       return -1;
1945     n = PGSIZE - (va - va0);
1946     if(n > len)
1947       n = len;
1948     memmove(pa0 + (va - va0), buf, n);
1949     len -= n;
1950     buf += n;
1951     va = va0 + PGSIZE;
1952   }
1953   return 0;
1954 }
1955 
1956 // Blank page.
1957 // Blank page.
1958 // Blank page.
1959 
1960 
1961 
1962 
1963 
1964 
1965 
1966 
1967 
1968 
1969 
1970 
1971 
1972 
1973 
1974 
1975 
1976 
1977 
1978 
1979 
1980 
1981 
1982 
1983 
1984 
1985 
1986 
1987 
1988 
1989 
1990 
1991 
1992 
1993 
1994 
1995 
1996 
1997 
1998 
1999 
