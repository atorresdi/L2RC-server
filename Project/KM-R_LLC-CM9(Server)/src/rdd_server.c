/* rdd_server.h  Implementation of the Robot Device Driver server module */

#include "rdd_server.h"
#include "protocol_tx.h"
#include <cstdlib>

#include "debug.h"

/* Extern variables */
extern Prx_Control c_prx;
extern Ptx_Control c_ptx;

/* Static routines */
static void Rds_Print_Config(Rds_Control *rdp)
{
	uint8_t n;
	
	Db_Print_Val('%', (rdp->period >> 8));	
	Db_Print_Val('%', rdp->period);
	Db_Print_Line("PERIOD");
	
	Db_Print_Val('$', rdp->dev_num);
	Db_Print_Line("DEV_NUM");
	
	for (n = 0; n < rdp->dev_num; n++)
	{
		uint8_t i;
		Db_Print_Val('%', rdp->dev[n].dev_id);
		Db_Print_Line("DEV_ID");
		
		Db_Print_Val('$', rdp->dev[n].inst_num);
		Db_Print_Line("INST_NUM");
		
		for (i = 0; i < rdp->dev[n].inst_num; i++)
			Db_Print_Val('*', rdp->dev[n].inst_id[i]);
		Db_Print_Line("INST_ID");
		
		Db_Print_Val('$', rdp->dev[n].param_wr_num);
		Db_Print_Line("PARAM_WR_NUM");
		
		if (rdp->dev[n].param_wr_num)
		{
			for (i = 0; i < rdp->dev[n].param_wr_num; i++)
				Db_Print_Val('*', rdp->dev[n].param_wr_addr[i]);
			Db_Print_Line("PARAM_WR_ADDR");
		};
		
		Db_Print_Val('$', rdp->dev[n].param_rd_num);
		Db_Print_Line("PARAM_RD_NUM");
		
		if (rdp->dev[n].param_wr_num)
		{
			for (i = 0; i < rdp->dev[n].param_rd_num; i++)
				Db_Print_Val('*', rdp->dev[n].param_rd_addr[i]);
			Db_Print_Line("PARAM_WR_ADDR");
			
			for (i = 0; i < rdp->dev[n].param_rd_num; i++)
				Db_Print_Val('*', rdp->dev[n].param_rd_per[i]);
			Db_Print_Line("PARAM_WR_PER");
		};
	};
}

/* Initialization */
void Rds_Define(Rds_Control *rdp)
{
	rdp->flags = 0;
	rdp->dev_ena_flags = 0;
	rdp->state = 0;
	rdp->dev = 0;
	rdp->dev_idx = 0;
	rdp->pkg_p = 0;
}

/* Robot Device Driver server parameters set up */
void Rds_Configure(Rds_Control *rdp)
{
	switch(rdp->state)
	{
		case RDS_WAIT_CONFIG_PKG:
			
			if(Prx_Pkg_Avail(&c_prx))
			{
				rdp->pkg_p = Prx_Get_Pkg(&c_prx);
				
				if (Prx_Get_Pkg_Type(rdp->pkg_p))			 /* package type != 0 */
				{
					Prx_Ckout_Curr_Pkg(&c_prx);
					rdp->pkg_p = 0;
					break;
				};
				
				rdp->pkg_dev_id = Prx_Get_Pkg_Dev_Id(rdp->pkg_p);
				rdp->state = RDS_STORE_CONFIG_PARAM;
			};
			
			break;
			
		case RDS_STORE_CONFIG_PARAM:
			
			if (!rdp->pkg_dev_id)							/* device identifier == 0 (global parameter) */
			{
				if (rdp->pkg_p->ptsf == P_RDS_PERIOD)
				{rdp->period = *((uint16_t *)rdp->pkg_p->data);	Db_Print_Val('%', (rdp->period >> 8));	Db_Print_Val('%', rdp->period);}
				else if (rdp->pkg_p->ptsf == P_RDS_DEV_NUM)
				{
					rdp->dev_num = *rdp->pkg_p->data;					Db_Print_Val('$', rdp->dev_num);
					rdp->dev = (Rds_Device *)malloc( (rdp->dev_num)*sizeof(Rds_Device));
				};				
			}
			else															/* device identifier != 0 (device parameter) */
			{
				if (rdp->pkg_p->ptsf == P_RDS_DEV_ID)
				{rdp->dev[rdp->dev_idx].dev_id = *rdp->pkg_p->data;			Db_Print_Val('%', rdp->dev[rdp->dev_idx].dev_id);}
				else if (rdp->pkg_p->ptsf == P_RDS_INST_NUM)
				{
					rdp->dev[rdp->dev_idx].inst_num = *rdp->pkg_p->data;	Db_Print_Val('$', rdp->dev[rdp->dev_idx].inst_num);
					rdp->dev[rdp->dev_idx].inst_id = (uint8_t *)malloc( (rdp->dev[rdp->dev_idx].inst_num)*sizeof(uint8_t));			Db_Print_Char('o');Db_Print_Char('k');
				}
				else if (rdp->pkg_p->ptsf == P_RDS_INST_ID)
				{
					uint8_t n = rdp->dev[rdp->dev_idx].inst_num;
					uint8_t *inst_id_p = rdp->dev[rdp->dev_idx].inst_id;
					uint8_t *pkg_data_p = rdp->pkg_p->data;
					
					for ( ; n; n--, inst_id_p++, pkg_data_p++)
						{*inst_id_p = *pkg_data_p;					Db_Print_Val('*', *inst_id_p);}
				}
				else if (rdp->pkg_p->ptsf == P_RDS_PARAM_WR_NUM)
				{
					rdp->dev[rdp->dev_idx].param_wr_num = *rdp->pkg_p->data;	Db_Print_Val('$', rdp->dev[rdp->dev_idx].param_wr_num);
					
					if (rdp->dev[rdp->dev_idx].param_wr_num)
					{
						rdp->dev[rdp->dev_idx].param_wr_addr = (uint8_t *)malloc( (rdp->dev[rdp->dev_idx].param_wr_num)*sizeof(uint8_t));			Db_Print_Char('o');Db_Print_Char('k');
					};
				}
				else if (rdp->pkg_p->ptsf == P_RDS_PARAM_WR_ADDR)
				{
					uint8_t n = rdp->dev[rdp->dev_idx].param_wr_num;
					uint8_t *param_wr_addr_p = rdp->dev[rdp->dev_idx].param_wr_addr;
					uint8_t *pkg_data_p = rdp->pkg_p->data;
					
					for ( ; n; n--, param_wr_addr_p++, pkg_data_p++)
						{*param_wr_addr_p = *pkg_data_p;					Db_Print_Val('*', *param_wr_addr_p);}
					
				}
				else if (rdp->pkg_p->ptsf == P_RDS_PARAM_RD_NUM)
				{
					rdp->dev[rdp->dev_idx].param_rd_num = *rdp->pkg_p->data;	Db_Print_Val('$', rdp->dev[rdp->dev_idx].param_rd_num);
					
					if (rdp->dev[rdp->dev_idx].param_rd_num)
					{
						rdp->dev[rdp->dev_idx].param_rd_addr = (uint8_t *)malloc( (rdp->dev[rdp->dev_idx].param_rd_num)*sizeof(uint8_t));			Db_Print_Char('o');Db_Print_Char('k');
						rdp->dev[rdp->dev_idx].param_rd_per = (uint8_t *)malloc( (rdp->dev[rdp->dev_idx].param_rd_num)*sizeof(uint8_t));			Db_Print_Char('o');Db_Print_Char('k');
					}
					else
					{
						(rdp->dev_idx)++;
					
						if (rdp->dev_idx >= rdp->dev_num)
						{
							#ifdef DEBUG_ENABLE
							Rds_Print_Config(rdp);
							#endif 
							rdp->dev_ena_flags |= F_RDS_CONFIGURED;
							rdp->state = RDS_PING;
							break;
						};
					};
				}
				else if (rdp->pkg_p->ptsf == P_RDS_PARAM_RD_ADDR)
				{
					uint8_t n = rdp->dev[rdp->dev_idx].param_wr_num;
					uint8_t *param_rd_addr_p = rdp->dev[rdp->dev_idx].param_rd_addr;
					uint8_t *pkg_data_p = rdp->pkg_p->data;
					
					for ( ; n; n--, param_rd_addr_p++, pkg_data_p++)
						{*param_rd_addr_p = *pkg_data_p;					Db_Print_Val('*', *param_rd_addr_p);}
				}
				else if (rdp->pkg_p->ptsf == P_RDS_PARAM_RD_PER)
				{
					uint8_t n = rdp->dev[rdp->dev_idx].param_wr_num;
					uint8_t *param_rd_per_p = rdp->dev[rdp->dev_idx].param_rd_per;
					uint8_t *pkg_data_p = rdp->pkg_p->data;
					
					for ( ; n; n--, param_rd_per_p++, pkg_data_p++)
						{*param_rd_per_p = *pkg_data_p;					Db_Print_Val('*', *param_rd_per_p);}
						
					if (rdp->dev[rdp->dev_idx].dev_id == RDS_DXL_AX_ID)
						rdp->dev_ena_flags |= F_RDS_DXL_AX_ENABLE;
					
					(rdp->dev_idx)++;
					
					if (rdp->dev_idx >= rdp->dev_num)
					{
						#ifdef DEBUG_ENABLE
						Rds_Print_Config(rdp);
						#endif 
						rdp->dev_ena_flags |= F_RDS_CONFIGURED;
						rdp->state = RDS_PING;
						break;
					};
				};
			}
			
			Prx_Ckout_Curr_Pkg(&c_prx);
			rdp->state = RDS_WAIT_CONFIG_PKG;
			
			break;
	};
}

