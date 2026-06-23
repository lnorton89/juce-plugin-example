import type { Env } from './env';
import { handleLemonWebhook } from './webhook';
import { handleActivate, handleDeactivate, handleValidate } from './api/activationRoutes';

export async function handleRequest(
  request: Request,
  env: Env,
  ctx: ExecutionContext
): Promise<Response> {
  const url = new URL(request.url);
  const method = request.method.toUpperCase();
  const path = url.pathname;

  switch (true) {
    case method === 'GET' && path === '/api/health':
      return handleHealth(env);

    case method === 'POST' && path === '/api/webhook/lemon-squeezy':
      return handleLemonWebhook(request, env, ctx);

    case method === 'POST' && path === '/api/v1/activate':
      return handleActivate(request, env);

    case method === 'POST' && path === '/api/v1/validate':
      return handleValidate(request, env);

    case method === 'POST' && path === '/api/v1/deactivate':
      return handleDeactivate(request, env);

    default:
      return new Response(JSON.stringify({ error: 'Not Found' }), {
        status: 404,
        headers: { 'Content-Type': 'application/json' },
      });
  }
}

async function handleHealth(env: Env): Promise<Response> {
  return new Response(
    JSON.stringify({
      status: 'ok',
      version: 1,
      environment: env.ENVIRONMENT || 'production',
    }),
    {
      status: 200,
      headers: { 'Content-Type': 'application/json' },
    }
  );
}
