KMOD= fsaa1655g
SRCS= fsaa1655g.c
SRCS+=	bus_if.h device_if.h pci_if.h

.include <bsd.kmod.mk>
