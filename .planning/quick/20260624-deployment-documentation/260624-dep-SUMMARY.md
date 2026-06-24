# Quick Task Summary: Deployment Documentation

**ID:** 260624-dep
**Date:** 2026-06-24
**Status:** complete

## Completed Work

We have successfully created a comprehensive **Deployment and Operations Guide** (`docs/deployment.md`) for the LumaScope Activation Service, covering deployment to both staging/testing (`preview`) and production (`production`) environments.

### Highlights:
1. **Multi-Environment Overview**: Contrast between `preview` (staging/testing) and `production` environments including worker names, databases, and routes.
2. **Bootstrapping & Setup**: Detailed instructions on using PowerShell bootstrap scripts, managing account credentials, and generating cryptographic Ed25519 keys for token signing.
3. **Deployment Secrets**: Best practices for secure secret configuration, specifically preventing any raw keys from being committed.
4. **Deploying and Migrations**: Walkthrough of using `deploy.ps1` to apply database migrations and deploy Cloudflare Workers.
5. **Lemon Squeezy Webhooks**: Detailed guide for webhook setup in both Test/Sandbox and Live/Production modes.
6. **Staging Verification**: Step-by-step procedures to validate webhook ingestion, query database records via Wrangler, and perform simulated activation tests.
7. **Promotion to Production**: Orderly flow for auditing, bumping versions, and deploying to the live production endpoint.
8. **Disaster Recovery**: Backing up and restoring Cloudflare D1 database state.
9. **Instant Rollbacks**: Commands to view past wrangler deployments and perform instantaneous rollbacks.
10. **Zero-Downtime Signing Key Rotation**: Detailed walkthrough on using a public key ring to rotate Ed25519 keys seamlessly without breaking existing user activations.

### Cross-references Updated:
- Added cross-reference to `docs/cloud-infrastructure.md` linking to the new `docs/deployment.md` for seamless operations.
