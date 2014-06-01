/* rdd_server.h  Implementation of the Robot Device Driver server module */

#include "timing.h"
#include "rdd_server.h"
#include "protocol_tx.h"
#include "dxl_ax.h"
#include "error.h"
#include <cstdlib>

#include "debug.h"

/* Extern variables */
extern struct Tm_Control c_time;
extern Prx_Control c_prx;
extern Ptx_Control c_ptx;
extern Dax_Control c_dax;
extern uint8_t dax_inst_pkg[DAX_INST_PKG_MAX_LEN];
extern uint8_t dax_stus_pkg[DAX_STUS_PKG_MAX_LEN];

/* Static variables */
uint8_t dxl_ax_tmp_data_len;
uint8_t *dxl_ax_tmp_data;

/* Static routines */
/* print the configuration parameters values */
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
		
		if (rdp->dev[n].param_rd_num)
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

/* initialize a device */
void Rds_Initialize_Device(Rds_Control *rdp, Rds_Device *dev)
{
	if (dev->dev_id == RDS_DXL_AX_ID)
	{
		rdp->dev_ena_flags |= F_RDS_DXL_AX_ENABLE;
		Dax_Define(&c_dax, dev->inst_num, dev->inst_id, dax_inst_pkg, dax_stus_pkg);
		dxl_ax_tmp_data_len = 2*(rdp->dev[rdp->dev_idx].inst_num);
		dxl_ax_tmp_data = (uint8_t *)malloc( dxl_ax_tmp_data_len*sizeof(uint8_t));
		Dax_Set_Stus_Rtn_Lvl(&c_dax, 1);
	}
}

/* Initialization */
void Rds_Define(Rds_Control *rdp)
{
	rdp->flags = 0;
	rdp->dev_ena_flags = 0;
	rdp->state = 0;
	rdp->next_state = 0;
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
					rdp->period = *((uint16_t *)rdp->pkg_p->data);
				else if (rdp->pkg_p->ptsf == P_RDS_DEV_NUM)
				{
					rdp->dev_num = *rdp->pkg_p->data;
					rdp->dev = (Rds_Device *)malloc( (rdp->dev_num)*sizeof(Rds_Device));
				};				
			}
			else															/* device identifier != 0 (device parameter) */
			{
				if (rdp->pkg_p->ptsf == P_RDS_DEV_ID)
					rdp->dev[rdp->dev_idx].dev_id = *rdp->pkg_p->data;
				else if (rdp->pkg_p->ptsf == P_RDS_INST_NUM)
				{
					rdp->dev[rdp->dev_idx].inst_num = *rdp->pkg_p->data;
					rdp->dev[rdp->dev_idx].inst_id = (uint8_t *)malloc( (rdp->dev[rdp->dev_idx].inst_num)*sizeof(uint8_t));
				}
				else if (rdp->pkg_p->ptsf == P_RDS_INST_ID)
				{
					uint8_t n = rdp->dev[rdp->dev_idx].inst_num;
					uint8_t *inst_id_p = rdp->dev[rdp->dev_idx].inst_id;
					uint8_t *pkg_data_p = rdp->pkg_p->data;
					
					for ( ; n; n--, inst_id_p++, pkg_data_p++)
						*inst_id_p = *pkg_data_p;
				}
				else if (rdp->pkg_p->ptsf == P_RDS_PARAM_WR_NUM)
				{
					rdp->dev[rdp->dev_idx].param_wr_num = *rdp->pkg_p->data;
					
					if (rdp->dev[rdp->dev_idx].param_wr_num)
						rdp->dev[rdp->dev_idx].param_wr_addr = (uint8_t *)malloc( (rdp->dev[rdp->dev_idx].param_wr_num)*sizeof(uint8_t));
				}
				else if (rdp->pkg_p->ptsf == P_RDS_PARAM_WR_ADDR)
				{
					uint8_t n = rdp->dev[rdp->dev_idx].param_wr_num;
					uint8_t *param_wr_addr_p = rdp->dev[rdp->dev_idx].param_wr_addr;
					uint8_t *pkg_data_p = rdp->pkg_p->data;
					
					for ( ; n; n--, param_wr_addr_p++, pkg_data_p++)
						*param_wr_addr_p = *pkg_data_p;
				}
				else if (rdp->pkg_p->ptsf == P_RDS_PARAM_RD_NUM)
				{
					rdp->dev[rdp->dev_idx].param_rd_num = *rdp->pkg_p->data;
					
					if (rdp->dev[rdp->dev_idx].param_rd_num)
					{
						rdp->dev[rdp->dev_idx].param_rd_addr = (uint8_t *)malloc( (rdp->dev[rdp->dev_idx].param_rd_num)*sizeof(uint8_t));
						rdp->dev[rdp->dev_idx].param_rd_per = (uint8_t *)malloc( (rdp->dev[rdp->dev_idx].param_rd_num)*sizeof(uint8_t));
					}
					else
					{
						Rds_Initialize_Device(rdp, &rdp->dev[rdp->dev_idx]);
						(rdp->dev_idx)++;
					
						if (rdp->dev_idx >= rdp->dev_num)
						{
							#ifdef DEBUG_ENABLE
								Rds_Print_Config(rdp);
							#endif 
							Prx_Ckout_Curr_Pkg(&c_prx);
							rdp->dev_idx = 0;
							rdp->state = RDS_INITIALIZE_DEVS;
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
					
					Rds_Initialize_Device(rdp, &rdp->dev[rdp->dev_idx]);
					(rdp->dev_idx)++;
					
					if (rdp->dev_idx >= rdp->dev_num)
					{
						#ifdef DEBUG_ENABLE
							Rds_Print_Config(rdp);
						#endif
						Prx_Ckout_Curr_Pkg(&c_prx);
						rdp->dev_idx = 0;
						rdp->state = RDS_INITIALIZE_DEVS;
						break;
					};
				};
			}
			
			Prx_Ckout_Curr_Pkg(&c_prx);
			rdp->state = RDS_WAIT_CONFIG_PKG;
			
			break;
			
		case RDS_INITIALIZE_DEVS:					/* Check ping result and start device initialization */
			
			if (rdp->dev_idx < rdp->dev_num)
			{
				if (rdp->dev[rdp->dev_idx].dev_id == RDS_DXL_AX_ID)
						rdp->state = RDS_DAX_PING;
			}
			else
			{
				Tm_Start_Period(&c_time, INSTR_EXEC_PERIOD_NUM, rdp->period);
				rdp->flags |= F_RDS_CONFIGURED;
				rdp->state = RDS_WAIT_INSTR_PKG;
			};
			
			break;
			
		case RDS_DAX_PING:
			
			Dax_Ping_Rqst(&c_dax);
			rdp->state = RDS_DAX_WAIT_RQST_COMPLETE;
			rdp->next_state = RDS_DAX_REDUCE_MOVING_SPEED;
		
			break;
		
		case RDS_DAX_REDUCE_MOVING_SPEED:
			
			{	
				/* reduce moving speed */
				uint8_t n = dxl_ax_tmp_data_len;
				uint8_t *dxl_ax_tmp_data_p = dxl_ax_tmp_data;
				
				/* set the moving speed value */
				for ( ; n; n--, dxl_ax_tmp_data_p++)
				{
					if (!(n % 2))
						*dxl_ax_tmp_data_p = 90;
					else
						*dxl_ax_tmp_data_p = 0;
				};
				
				/* Request a write operation to modify the moving speed value */
				Dax_Write_Rqst(&c_dax, DAX_MOVING_SPEED_ADDR, 1, dxl_ax_tmp_data);
					
				rdp->state = RDS_DAX_WAIT_RQST_COMPLETE;
				rdp->next_state = RDS_DAX_SET_INIT_POS;
			};
			
			break;
			
		case RDS_DAX_SET_INIT_POS:
			
			if (Prx_Pkg_Avail(&c_prx))
			{
				rdp->pkg_p = Prx_Get_Pkg(&c_prx);
				
				if (Prx_Get_Pkg_Type(rdp->pkg_p) != PRX_INSTRUCTION_PKG)
				{
					Prx_Ckout_Curr_Pkg(&c_prx);
					break;
				};
				
				/* Request a write operation to set the actuators initial position */
				Dax_Write_Rqst(&c_dax, DAX_GOAL_POSITION_ADDR, rdp->pkg_p->opts & 0x03, rdp->pkg_p->data);
				
				Tm_Start_Timeout(&c_time, RDS_DAX_INIT_POS_TOUT_NUM, RDS_DAX_INIT_POS_TOUT_VAL);
				
				rdp->state = RDS_DAX_WAIT_RQST_COMPLETE;
				rdp->next_state = RDS_DAX_WAIT_MOVEMENT_END;
			};
			
			break;
			
		case RDS_DAX_WAIT_MOVEMENT_END:
			
			if (Tm_Timeout_Complete(&c_time, RDS_DAX_INIT_POS_TOUT_NUM))
			{
					Prx_Ckout_Curr_Pkg(&c_prx);
					rdp->state = RDS_DAX_SET_MAX_MOVING_SPEED;
			};
			
			break;
			
		case RDS_DAX_SET_MAX_MOVING_SPEED:
			
			{	
				/* set maximun moving speed */
				uint8_t n = dxl_ax_tmp_data_len;
				uint8_t *dxl_ax_tmp_data_p = dxl_ax_tmp_data;
				
				/* set the moving speed value */
				for ( ; n; n--, dxl_ax_tmp_data_p++)
					*dxl_ax_tmp_data_p = 0;
				
				/* Request a write operation to modify the moving speed value */
				Dax_Write_Rqst(&c_dax, DAX_MOVING_SPEED_ADDR, 1, dxl_ax_tmp_data);
				
				rdp->state = RDS_DAX_WAIT_RQST_COMPLETE;
				rdp->next_state = RDS_INITIALIZE_DEVS;
			};
			
			break;
			
		case RDS_DAX_WAIT_RQST_COMPLETE:
			
			if (Dax_Rqst_Complete(&c_dax))
			{
				if (rdp->next_state == RDS_INITIALIZE_DEVS)
					(rdp->dev_idx)++;
				
				rdp->state = rdp->next_state;
			}
			else if (Dax_Err(&c_dax))
			{
				rdp->flags |= F_RDS_CONFIGURED;
				rdp->state = RDS_WAIT_TOKEN;
			};
			
			break;
	};
}

void Rds_Process(Rds_Control *rdp)
{
	switch (rdp->state)
	{
		case RDS_WAIT_INSTR_PKG:
			
			if (Prx_Pkg_Avail(&c_prx))
			{
				rdp->pkg_p = Prx_Get_Pkg(&c_prx);
				
				if (Prx_Get_Pkg_Type(rdp->pkg_p) != PRX_INSTRUCTION_PKG)
				{
					Prx_Ckout_Curr_Pkg(&c_prx);
					break;
				};
				
				if (Tm_Period_Complete(&c_time, INSTR_EXEC_PERIOD_NUM))
				{
					Tm_Clean_Period(&c_time, INSTR_EXEC_PERIOD_NUM);
					/* Request a write operation */
					Dax_Write_Rqst(&c_dax, DAX_GOAL_POSITION_ADDR, rdp->pkg_p->opts & 0x03, rdp->pkg_p->data);
					
					Tm_Start_Timeout(&c_time, RDS_DAX_INIT_POS_TOUT_NUM, RDS_DAX_INIT_POS_TOUT_VAL);
					
					rdp->state = RDS_DAX_WAIT_RQST_COMPLETE;
				}
				else
					rdp->state = RDS_DAX_WAIT_PERIOD;
			};
			
			break;
			
		case RDS_DAX_WAIT_PERIOD:
				
			if (Tm_Period_Complete(&c_time, INSTR_EXEC_PERIOD_NUM))
			{
				Tm_Clean_Period(&c_time, INSTR_EXEC_PERIOD_NUM);
				/* Request a write operation */
				Dax_Write_Rqst(&c_dax, DAX_GOAL_POSITION_ADDR, rdp->pkg_p->opts & 0x03, rdp->pkg_p->data);
				
				Tm_Start_Timeout(&c_time, RDS_DAX_INIT_POS_TOUT_NUM, RDS_DAX_INIT_POS_TOUT_VAL);
				
				rdp->state = RDS_DAX_WAIT_RQST_COMPLETE;
			};
			
			break;
			
		case RDS_WAIT_TOKEN:
			
			if (Prx_Cmd_Avail(&c_prx))
			{
				uint8_t cmd = Prx_Get_Cmd(&c_prx);
				
				if (cmd == PRX_TOKEN)
				{
					if (Dax_Err(&c_dax))
					{
						Error *dax_err = Dax_Get_Err(&c_dax);
						uint8_t pkg_opts = Ptx_Set_Pkg_Opts(PRX_ERROR_PKG, RDS_DXL_AX_ID, 0);
						
						Ptx_Add_Pkg_Rqst(&c_ptx, 1, pkg_opts, dax_err->dev_instance, &dax_err->err_flags);
						rdp->state = 255;
						#ifdef RDS_DEBUG_ENABLE
							Db_Print_Val('-', dax_err->dev_instance);
							Db_Print_Val('-', dax_err->err_flags);
						#endif
					}
					else
					{
						Ptx_Add_Cmd_Rqst(&c_ptx, PRX_TOKEN);
						rdp->state = RDS_WAIT_INSTR_PKG;
					};
				};
			};
			
			break;
			
		case RDS_DAX_WAIT_RQST_COMPLETE:
			
			if (Dax_Rqst_Complete(&c_dax) || Dax_Err(&c_dax))
			{
				Prx_Ckout_Curr_Pkg(&c_prx);
				rdp->state = RDS_WAIT_TOKEN;
			};
			
			break;
	};
}
