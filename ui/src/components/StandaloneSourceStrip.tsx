import { useState } from 'react';
import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import FormControl from '@mui/material/FormControl';
import InputLabel from '@mui/material/InputLabel';
import MenuItem from '@mui/material/MenuItem';
import Select from '@mui/material/Select';
import Typography from '@mui/material/Typography';
import StopIcon from '@mui/icons-material/Stop';
import type { BridgeStatus, SourceDescriptor, SourceMode } from '../bridge/protocol';

export interface StandaloneSourceStripProps {
  bridge: BridgeStatus;
  onSourceSelect: (mode: string, sourceId: string) => void;
  onSourceStop: () => void;
}

function statusText(state: string, sourceName: string, code: string): string {
  switch (state) {
    case 'stopped':
      return 'Choose source';
    case 'starting':
      return 'Starting\u2026';
    case 'active':
      return `Active \u2014 ${sourceName}`;
    case 'silent':
      return `No signal detected \u2014 ${sourceName}`;
    case 'error':
      return `Error: ${code}`;
    default:
      return '';
  }
}

function statusColor(state: string): string {
  switch (state) {
    case 'active':
      return 'status.ready';
    case 'silent':
      return 'text.secondary';
    case 'error':
      return 'status.destructive';
    default:
      return 'text.secondary';
  }
}

export function StandaloneSourceStrip({ bridge, onSourceSelect, onSourceStop }: StandaloneSourceStripProps) {
  if (bridge.state !== 'ready') return null;
  if (bridge.hostInfo.hostMode !== 'Standalone') return null;

  const sourceList = bridge.sourceList;
  const sourceStateVal = bridge.sourceState;

  // Local mode selector — determines which source list is shown in the dropdown
  // Defaults to the currently selected source's mode, or InputDevice
  const [selectedMode, setSelectedMode] = useState<SourceMode>(
    sourceStateVal?.mode ?? 'InputDevice',
  );

  // Keep selectedMode in sync when source state changes (e.g., after select from another trigger)
  const effectiveMode: SourceMode = sourceStateVal?.mode ?? selectedMode;

  const availableSources: SourceDescriptor[] = effectiveMode === 'InputDevice'
    ? (sourceList?.inputDevices ?? [])
    : (sourceList?.systemOutputs ?? []);

  const isActive = sourceStateVal != null && (sourceStateVal.state === 'active' || sourceStateVal.state === 'silent');
  const canStop = isActive;

  function handleModeChange(mode: string) {
    const newMode = mode as SourceMode;
    setSelectedMode(newMode);
    // Mode changes are local only — user must pick a specific source to trigger onSourceSelect
  }

  function handleSourceChange(sourceId: string) {
    onSourceSelect(effectiveMode === 'InputDevice' ? 'InputDevice' : 'SystemOutput', sourceId);
  }

  return (
    <Box
      component="section"
      aria-label="Source controls"
      sx={{
        display: 'flex',
        alignItems: 'center',
        gap: 2,
        px: { xs: 3, sm: 5 },
        py: 1,
        borderBottom: 1,
        borderColor: 'border',
        bgcolor: 'background.paper',
        flexWrap: 'wrap',
        minWidth: 0,
      }}
    >
      <FormControl size="small" sx={{ minWidth: 140 }}>
        <InputLabel id="source-mode-label">Source mode</InputLabel>
        <Select
          labelId="source-mode-label"
          aria-label="Source mode"
          value={effectiveMode}
          label="Source mode"
          onChange={(e) => handleModeChange(e.target.value)}
          disabled={isActive}
        >
          <MenuItem value="InputDevice">Input Device</MenuItem>
          <MenuItem value="SystemOutput">System Output</MenuItem>
        </Select>
      </FormControl>

      <FormControl size="small" sx={{ minWidth: 200 }}>
        <InputLabel id="source-select-label">Select source</InputLabel>
        <Select
          labelId="source-select-label"
          aria-label="Select source"
          value={sourceStateVal?.selectedSourceId ?? ''}
          label="Select source"
          onChange={(e) => handleSourceChange(e.target.value)}
          disabled={isActive}
        >
          {availableSources.map((source) => (
            <MenuItem key={source.id} value={source.id}>
              {source.displayName}
            </MenuItem>
          ))}
        </Select>
      </FormControl>

      <Typography
        role="status"
        aria-live="polite"
        variant="body2"
        noWrap
        sx={{ color: sourceStateVal ? statusColor(sourceStateVal.state) : 'text.secondary', flexShrink: 0 }}
      >
        {sourceStateVal ? statusText(sourceStateVal.state, sourceStateVal.selectedSourceName, sourceStateVal.code) : 'No source state'}
      </Typography>

      {canStop && (
        <Button
          size="small"
          variant="outlined"
          color="error"
          startIcon={<StopIcon aria-hidden="true" />}
          onClick={onSourceStop}
          aria-label="Stop source"
          sx={{ flexShrink: 0, minHeight: 32 }}
        >
          Stop
        </Button>
      )}
    </Box>
  );
}
