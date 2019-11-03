1400 // Mutual exclusion spin locks.
1401 
1402 #include "types.h"
1403 #include "defs.h"
1404 #include "param.h"
1405 #include "x86.h"
1406 #include "memlayout.h"
1407 #include "mmu.h"
1408 #include "proc.h"
1409 #include "spinlock.h"
1410 
1411 void
1412 initlock(struct spinlock *lk, char *name)
1413 {
1414   lk->name = name;
1415   lk->locked = 0;
1416   lk->cpu = 0;
1417 }
1418 
1419 // Acquire the lock.
1420 // Loops (spins) until the lock is acquired.
1421 // Holding a lock for a long time may cause
1422 // other CPUs to waste time spinning to acquire it.
1423 void
1424 acquire(struct spinlock *lk)
1425 {
1426   pushcli(); // disable interrupts to avoid deadlock.
1427   if(holding(lk))
1428     panic("acquire");
1429 
1430   // The xchg is atomic.
1431   while(xchg(&lk->locked, 1) != 0)
1432     ;
1433 
1434   // Tell the C compiler and the processor to not move loads or stores
1435   // past this point, to ensure that the critical section's memory
1436   // references happen after the lock is acquired.
1437   __sync_synchronize();
1438 
1439   // Record info about lock acquisition for debugging.
1440   lk->cpu = mycpu();
1441   getcallerpcs(&lk, lk->pcs);
1442 }
1443 
1444 
1445 
1446 
1447 
1448 
1449 
1450 // Release the lock.
1451 void
1452 release(struct spinlock *lk)
1453 {
1454   if(!holding(lk))
1455     panic("release");
1456 
1457   lk->pcs[0] = 0;
1458   lk->cpu = 0;
1459 
1460   // Tell the C compiler and the processor to not move loads or stores
1461   // past this point, to ensure that all the stores in the critical
1462   // section are visible to other cores before the lock is released.
1463   // Both the C compiler and the hardware may re-order loads and
1464   // stores; __sync_synchronize() tells them both not to.
1465   __sync_synchronize();
1466 
1467   // Release the lock, equivalent to lk->locked = 0.
1468   // This code can't use a C assignment, since it might
1469   // not be atomic. A real OS would use C atomics here.
1470   asm volatile("movl $0, %0" : "+m" (lk->locked) : );
1471 
1472   popcli();
1473 }
1474 
1475 // Record the current call stack in pcs[] by following the %ebp chain.
1476 void
1477 getcallerpcs(void *v, uint pcs[])
1478 {
1479   uint *ebp;
1480   int i;
1481 
1482   ebp = (uint*)v - 2;
1483   for(i = 0; i < 10; i++){
1484     if(ebp == 0 || ebp < (uint*)KERNBASE || ebp == (uint*)0xffffffff)
1485       break;
1486     pcs[i] = ebp[1];     // saved %eip
1487     ebp = (uint*)ebp[0]; // saved %ebp
1488   }
1489   for(; i < 10; i++)
1490     pcs[i] = 0;
1491 }
1492 
1493 
1494 
1495 
1496 
1497 
1498 
1499 
1500 // Check whether this cpu is holding the lock.
1501 int
1502 holding(struct spinlock *lock)
1503 {
1504   int r;
1505   pushcli();
1506   r = lock->locked && lock->cpu == mycpu();
1507   popcli();
1508   return r;
1509 }
1510 
1511 
1512 // Pushcli/popcli are like cli/sti except that they are matched:
1513 // it takes two popcli to undo two pushcli.  Also, if interrupts
1514 // are off, then pushcli, popcli leaves them off.
1515 
1516 void
1517 pushcli(void)
1518 {
1519   int eflags;
1520 
1521   eflags = readeflags();
1522   cli();
1523   if(mycpu()->ncli == 0)
1524     mycpu()->intena = eflags & FL_IF;
1525   mycpu()->ncli += 1;
1526 }
1527 
1528 void
1529 popcli(void)
1530 {
1531   if(readeflags()&FL_IF)
1532     panic("popcli - interruptible");
1533   if(--mycpu()->ncli < 0)
1534     panic("popcli");
1535   if(mycpu()->ncli == 0 && mycpu()->intena)
1536     sti();
1537 }
1538 
1539 
1540 
1541 
1542 
1543 
1544 
1545 
1546 
1547 
1548 
1549 
