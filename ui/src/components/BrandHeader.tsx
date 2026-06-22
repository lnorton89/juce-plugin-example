import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';

export interface BrandHeaderProps { mode?: 'VST3' | 'Standalone' }

export function BrandHeader({ mode }: BrandHeaderProps) {
  return (
    <Box
      component="header"
      sx={{
        minWidth: 0,
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'space-between',
        px: 5,
        borderBottom: 1,
        borderColor: 'border',
        bgcolor: 'background.paper',
        '@media (max-width:639px)': { px: 3 },
      }}
    >
      <Box sx={{ display: 'flex', alignItems: 'center', gap: 2, minWidth: 0 }}>
        <Box aria-hidden="true" sx={{ width: 4, height: 20, borderRadius: 1, bgcolor: 'primary.main' }} />
        <Typography component="h1" variant="h1" noWrap>LumaScope</Typography>
        <Typography color="text.secondary" noWrap sx={{ display: { xs: 'none', sm: 'block' } }}>
          Spectrum analyzer
        </Typography>
      </Box>
      {mode && <Typography aria-label="Product mode" variant="subtitle2" noWrap>{mode}</Typography>}
    </Box>
  );
}
