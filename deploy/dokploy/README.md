# Mira Web on Dokploy Cloud

This deployment is designed for Dokploy Cloud to own the running service.
Dokploy builds a small Caddy image from this repository, routes the domain via
its Domains tab, tracks deployments, and can auto-deploy from GitHub.

Mira's browser build is generated locally by Dawn under `out/mira-release/wasm`.
The generated web assets are copied into `web-dist/` and committed so Dokploy
does not need to build Dawn on the 1 GB VPS.

## Update Web Assets

```sh
ant run dist:web
```

Useful override when the release output is already current:

```sh
BUILD=0 ant run dist:web
```

Commit the updated `web-dist/` files, then push to GitHub. With Auto Deploy
enabled, Dokploy will build and deploy the updated Caddy image.

## Dokploy Cloud Setup

Create a Docker Compose app:

- Source: GitHub
- Repository: `hystericca/mira`
- Branch: `main`
- Compose path: `deploy/dokploy/docker-compose.yml`
- Deployment type: Docker Compose, not Stack
- Server: the DigitalOcean `aqua` server

In the app's Domains tab, add:

- Host: `mira.hystericca.dev`
- Service: `web`
- Container Port: `8080`
- HTTPS: enabled

Enable Auto Deploy if you want pushes to `main` to redeploy automatically.

## One-Time Migration From Manual Smoke Test

If the manually started smoke-test container is still running, stop it before
deploying the Dokploy Cloud app:

```sh
ssh aqua 'cd /opt/mira-web/compose && docker compose down'
```

After that, the Mira container should be managed from Dokploy Cloud.

## DNS

Point the `mira.hystericca.dev` A record at the VPS public IPv4:

```text
146.190.141.170
```
