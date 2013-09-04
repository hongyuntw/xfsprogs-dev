/*
 * Copyright (c) 2000-2001,2004-2005 Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software Foundation,
 * Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
 * maxtrres.c
 *
 * Compute the maximum transaction reservation for a legal combination
 * of sector size, block size, inode size, directory version, and
 * directory block size.
 */

#include <xfs/libxfs.h>
#include "xfs_mkfs.h"

static void
max_attrsetm_trans_res_adjust(
	xfs_mount_t			*mp)
{
	int				local;
	int				size;
	int				nblks;
	int				res;

	/*
	 * Determine space the maximal sized attribute will use,
	 * to calculate the largest reservation size needed.
	 */
	size = libxfs_attr_leaf_newentsize(MAXNAMELEN, 64 * 1024,
						mp->m_sb.sb_blocksize, &local);
	ASSERT(!local);
	nblks = XFS_DAENTER_SPACE_RES(mp, XFS_ATTR_FORK);
	nblks += XFS_B_TO_FSB(mp, size);
	nblks += XFS_NEXTENTADD_SPACE_RES(mp, size, XFS_ATTR_FORK);
	res = M_RES(mp)->tr_attrsetm.tr_logres +
	      M_RES(mp)->tr_attrsetrt.tr_logres * nblks;
	M_RES(mp)->tr_attrsetm.tr_logres = res;
}

static int
max_trans_res_by_mount(
	struct xfs_mount	*mp)
{
	struct xfs_trans_resv	*tr = &mp->m_resv;
	struct xfs_trans_res	*p;
	struct xfs_trans_res	rval = {0};

	for (p = (struct xfs_trans_res *)tr;
	     p < (struct xfs_trans_res *)(tr + 1); p++) {
		if (p->tr_logres > rval.tr_logres)
			rval = *p;
	}
	return rval.tr_logres;
}

int
max_trans_res(
	int		crcs_enabled,
	int		dirversion,
	int		sectorlog,
	int		blocklog,
	int		inodelog,
	int		dirblocklog)
{
	xfs_sb_t	*sbp;
	xfs_mount_t	mount;
	int		maxres, maxfsb;

	memset(&mount, 0, sizeof(mount));
	sbp = &mount.m_sb;
	sbp->sb_magicnum = XFS_SB_MAGIC;
	sbp->sb_sectlog = sectorlog;
	sbp->sb_sectsize = 1 << sbp->sb_sectlog;
	sbp->sb_blocklog = blocklog;
	sbp->sb_blocksize = 1 << blocklog;
	sbp->sb_agblocks = XFS_AG_MIN_BYTES / (1 << blocklog);
	sbp->sb_inodelog = inodelog;
	sbp->sb_inopblog = blocklog - inodelog;
	sbp->sb_inodesize = 1 << inodelog;
	sbp->sb_inopblock = 1 << (blocklog - inodelog);
	sbp->sb_dirblklog = dirblocklog - blocklog;
	sbp->sb_versionnum =
			(crcs_enabled ? XFS_SB_VERSION_5 : XFS_SB_VERSION_4) |
			(dirversion == 2 ? XFS_SB_VERSION_DIRV2BIT : 0);

	libxfs_mount(&mount, sbp, 0,0,0,0);
	max_attrsetm_trans_res_adjust(&mount);
	maxres = max_trans_res_by_mount(&mount);
	maxfsb = XFS_B_TO_FSB(&mount, maxres);
	libxfs_umount(&mount);

#if 0
	printf("#define\tMAXTRRES_S%d_B%d_I%d_D%d_V%d\t%lld\n",
		sectorlog, blocklog, inodelog, dirblocklog, dirversion, maxfsb);
#endif

	return maxfsb;
}
