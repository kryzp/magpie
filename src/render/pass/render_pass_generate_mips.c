
R_PASS_RECORD_DEF(R_GenerateMipsPassFn)
{
	G_CmdBuffer *cmd = ctx->cmd;
	
	const R_GenerateMipsPassData *user_data = ctx->user_data;

	G_CmdGenerateMipmaps(cmd, user_data->texture);
}
