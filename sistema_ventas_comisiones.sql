-- ============================================================
-- Sistema de Ventas y Comisiones - SQL Server Schema
-- ============================================================

IF DB_ID('SistemaVentas') IS NOT NULL
    ALTER DATABASE SistemaVentas SET SINGLE_USER WITH ROLLBACK IMMEDIATE;
GO
IF DB_ID('SistemaVentas') IS NOT NULL
    DROP DATABASE SistemaVentas;
GO
CREATE DATABASE SistemaVentas;
GO
USE SistemaVentas;
GO

-- ============================================================
-- 1. ROLES
-- ============================================================
CREATE TABLE roles (
    id          INT IDENTITY(1,1) NOT NULL,
    nombre      NVARCHAR(50)  NOT NULL,
    descripcion NVARCHAR(200) NULL,
    CONSTRAINT PK_roles PRIMARY KEY CLUSTERED (id),
    CONSTRAINT UQ_roles_nombre UNIQUE (nombre)
);
GO

-- ============================================================
-- 2. USUARIOS
-- ============================================================
CREATE TABLE usuarios (
    id                INT IDENTITY(1,1) NOT NULL,
    username          NVARCHAR(50)  NOT NULL,
    password_hash     NVARCHAR(256) NOT NULL,
    email             NVARCHAR(100) NOT NULL,
    id_rol            INT           NOT NULL,
    activo            BIT           NOT NULL CONSTRAINT DF_usuarios_activo DEFAULT 1,
    intentos_fallidos INT           NOT NULL CONSTRAINT DF_usuarios_intentos DEFAULT 0,
    bloqueado_hasta   DATETIME      NULL,
    ultimo_acceso     DATETIME      NULL,
    CONSTRAINT PK_usuarios PRIMARY KEY CLUSTERED (id),
    CONSTRAINT UQ_usuarios_username UNIQUE (username),
    CONSTRAINT UQ_usuarios_email UNIQUE (email),
    CONSTRAINT FK_usuarios_roles FOREIGN KEY (id_rol) REFERENCES roles(id)
);
GO

-- ============================================================
-- 3. PERMISOS
-- ============================================================
CREATE TABLE permisos (
    id          INT IDENTITY(1,1) NOT NULL,
    nombre      NVARCHAR(100) NOT NULL,
    descripcion NVARCHAR(200) NULL,
    CONSTRAINT PK_permisos PRIMARY KEY CLUSTERED (id),
    CONSTRAINT UQ_permisos_nombre UNIQUE (nombre)
);
GO

-- ============================================================
-- 4. ROLES_PERMISOS
-- ============================================================
CREATE TABLE roles_permisos (
    id_rol     INT NOT NULL,
    id_permiso INT NOT NULL,
    CONSTRAINT PK_roles_permisos PRIMARY KEY CLUSTERED (id_rol, id_permiso),
    CONSTRAINT FK_rolespermisos_roles FOREIGN KEY (id_rol) REFERENCES roles(id),
    CONSTRAINT FK_rolespermisos_permisos FOREIGN KEY (id_permiso) REFERENCES permisos(id)
);
GO

-- ============================================================
-- 5. SUPERVISORES (created before vendedores for FK)
-- ============================================================
CREATE TABLE supervisores (
    id          INT IDENTITY(1,1) NOT NULL,
    id_vendedor INT NOT NULL,
    CONSTRAINT PK_supervisores PRIMARY KEY CLUSTERED (id),
    CONSTRAINT UQ_supervisores_vendedor UNIQUE (id_vendedor)
);
GO

-- ============================================================
-- 6. VENDEDORES
-- ============================================================
CREATE TABLE vendedores (
    id                 INT IDENTITY(1,1) NOT NULL,
    id_usuario         INT            NOT NULL,
    nombre             NVARCHAR(100)  NOT NULL,
    identificacion     NVARCHAR(20)   NOT NULL,
    telefono           NVARCHAR(20)   NULL,
    email              NVARCHAR(100)  NULL,
    fecha_contratacion DATE           NOT NULL,
    tipo_contrato      NVARCHAR(30)   NOT NULL,
    salario_base       DECIMAL(18,2)  NOT NULL,
    comision_global    DECIMAL(5,2)   NOT NULL CONSTRAINT DF_vendedores_comision_global DEFAULT 0,
    id_supervisor      INT            NULL,
    activo             BIT            NOT NULL CONSTRAINT DF_vendedores_activo DEFAULT 1,
    CONSTRAINT PK_vendedores PRIMARY KEY CLUSTERED (id),
    CONSTRAINT UQ_vendedores_identificacion UNIQUE (identificacion),
    CONSTRAINT UQ_vendedores_usuario UNIQUE (id_usuario),
    CONSTRAINT FK_vendedores_usuarios FOREIGN KEY (id_usuario) REFERENCES usuarios(id),
    CONSTRAINT FK_vendedores_supervisor FOREIGN KEY (id_supervisor) REFERENCES supervisores(id),
    CONSTRAINT CK_vendedores_tipo_contrato CHECK (tipo_contrato IN (N'Indefinido', N'Termino fijo', N'Obra labor', N'Prestacion de servicios')),
    CONSTRAINT CK_vendedores_salario_base CHECK (salario_base >= 0),
    CONSTRAINT CK_vendedores_comision_global CHECK (comision_global >= 0 AND comision_global <= 100)
);
GO

-- Add FK from supervisores to vendedores
ALTER TABLE supervisores
    ADD CONSTRAINT FK_supervisores_vendedores FOREIGN KEY (id_vendedor) REFERENCES vendedores(id);
GO

-- ============================================================
-- 7. ZONAS
-- ============================================================
CREATE TABLE zonas (
    id          INT IDENTITY(1,1) NOT NULL,
    nombre      NVARCHAR(100) NOT NULL,
    descripcion NVARCHAR(200) NULL,
    activo      BIT           NOT NULL CONSTRAINT DF_zonas_activo DEFAULT 1,
    CONSTRAINT PK_zonas PRIMARY KEY CLUSTERED (id),
    CONSTRAINT UQ_zonas_nombre UNIQUE (nombre)
);
GO

-- ============================================================
-- 8. VENDEDORES_ZONAS
-- ============================================================
CREATE TABLE vendedores_zonas (
    id_vendedor INT NOT NULL,
    id_zona     INT NOT NULL,
    CONSTRAINT PK_vendedores_zonas PRIMARY KEY CLUSTERED (id_vendedor, id_zona),
    CONSTRAINT FK_vz_vendedores FOREIGN KEY (id_vendedor) REFERENCES vendedores(id),
    CONSTRAINT FK_vz_zonas FOREIGN KEY (id_zona) REFERENCES zonas(id)
);
GO

-- ============================================================
-- 9. CONTACTOS_EMERGENCIA
-- ============================================================
CREATE TABLE contactos_emergencia (
    id          INT IDENTITY(1,1) NOT NULL,
    id_vendedor INT           NOT NULL,
    nombre      NVARCHAR(100) NOT NULL,
    telefono    NVARCHAR(20)  NOT NULL,
    parentesco  NVARCHAR(50)  NOT NULL,
    CONSTRAINT PK_contactos_emergencia PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_contactos_vendedores FOREIGN KEY (id_vendedor) REFERENCES vendedores(id)
);
GO

-- ============================================================
-- 10. DATOS_BANCARIOS
-- ============================================================
CREATE TABLE datos_bancarios (
    id             INT IDENTITY(1,1) NOT NULL,
    id_vendedor    INT           NOT NULL,
    banco          NVARCHAR(100) NOT NULL,
    tipo_cuenta    NVARCHAR(30)  NOT NULL,
    numero_cuenta  NVARCHAR(30)  NOT NULL,
    activo         BIT           NOT NULL CONSTRAINT DF_datosbancarios_activo DEFAULT 1,
    CONSTRAINT PK_datos_bancarios PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_datosbancarios_vendedores FOREIGN KEY (id_vendedor) REFERENCES vendedores(id),
    CONSTRAINT CK_datosbancarios_tipo_cuenta CHECK (tipo_cuenta IN (N'Ahorro', N'Corriente', N'Daviplata', N'Movii'))
);
GO

-- ============================================================
-- 11. CLIENTES
-- ============================================================
CREATE TABLE clientes (
    id             INT IDENTITY(1,1) NOT NULL,
    nombre         NVARCHAR(150)  NOT NULL,
    identificacion NVARCHAR(20)   NOT NULL,
    telefono       NVARCHAR(20)   NULL,
    email          NVARCHAR(100)  NULL,
    direccion      NVARCHAR(200)  NULL,
    activo         BIT            NOT NULL CONSTRAINT DF_clientes_activo DEFAULT 1,
    CONSTRAINT PK_clientes PRIMARY KEY CLUSTERED (id),
    CONSTRAINT UQ_clientes_identificacion UNIQUE (identificacion)
);
GO

-- ============================================================
-- 12. CATEGORIAS
-- ============================================================
CREATE TABLE categorias (
    id          INT IDENTITY(1,1) NOT NULL,
    nombre      NVARCHAR(100) NOT NULL,
    descripcion NVARCHAR(200) NULL,
    activo      BIT           NOT NULL CONSTRAINT DF_categorias_activo DEFAULT 1,
    CONSTRAINT PK_categorias PRIMARY KEY CLUSTERED (id),
    CONSTRAINT UQ_categorias_nombre UNIQUE (nombre)
);
GO

-- ============================================================
-- 13. PRODUCTOS
-- ============================================================
CREATE TABLE productos (
    id                   INT IDENTITY(1,1) NOT NULL,
    nombre               NVARCHAR(150)  NOT NULL,
    descripcion          NVARCHAR(500)  NULL,
    id_categoria         INT            NOT NULL,
    precio               DECIMAL(18,2)  NOT NULL,
    costo                DECIMAL(18,2)  NOT NULL,
    comision_porcentaje  DECIMAL(5,2)   NOT NULL CONSTRAINT DF_productos_comision DEFAULT 0,
    comision_especial    DECIMAL(18,2)  NULL,
    stock                INT            NOT NULL CONSTRAINT DF_productos_stock DEFAULT 0,
    activo               BIT            NOT NULL CONSTRAINT DF_productos_activo DEFAULT 1,
    CONSTRAINT PK_productos PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_productos_categorias FOREIGN KEY (id_categoria) REFERENCES categorias(id),
    CONSTRAINT CK_productos_precio CHECK (precio >= 0),
    CONSTRAINT CK_productos_costo CHECK (costo >= 0),
    CONSTRAINT CK_productos_comision CHECK (comision_porcentaje >= 0 AND comision_porcentaje <= 100),
    CONSTRAINT CK_productos_stock CHECK (stock >= 0)
);
GO

-- ============================================================
-- 14. COTIZACIONES
-- ============================================================
CREATE TABLE cotizaciones (
    id             INT IDENTITY(1,1) NOT NULL,
    id_cliente     INT            NOT NULL,
    id_vendedor    INT            NOT NULL,
    fecha          DATETIME       NOT NULL CONSTRAINT DF_cotizaciones_fecha DEFAULT GETDATE(),
    subtotal       DECIMAL(18,2)  NOT NULL,
    impuesto       DECIMAL(18,2)  NOT NULL,
    total          DECIMAL(18,2)  NOT NULL,
    estado         NVARCHAR(20)   NOT NULL CONSTRAINT DF_cotizaciones_estado DEFAULT N'Pendiente',
    observaciones  NVARCHAR(500)  NULL,
    CONSTRAINT PK_cotizaciones PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_cotizaciones_clientes FOREIGN KEY (id_cliente) REFERENCES clientes(id),
    CONSTRAINT FK_cotizaciones_vendedores FOREIGN KEY (id_vendedor) REFERENCES vendedores(id),
    CONSTRAINT CK_cotizaciones_estado CHECK (estado IN (N'Pendiente', N'Aprobada', N'Rechazada', N'Convertida', N'Vencida')),
    CONSTRAINT CK_cotizaciones_subtotal CHECK (subtotal >= 0),
    CONSTRAINT CK_cotizaciones_impuesto CHECK (impuesto >= 0),
    CONSTRAINT CK_cotizaciones_total CHECK (total >= 0)
);
GO

-- ============================================================
-- 15. DETALLE_COTIZACIONES
-- ============================================================
CREATE TABLE detalle_cotizaciones (
    id                INT IDENTITY(1,1) NOT NULL,
    id_cotizacion     INT           NOT NULL,
    id_producto       INT           NOT NULL,
    cantidad          INT           NOT NULL,
    precio_unitario   DECIMAL(18,2) NOT NULL,
    subtotal          DECIMAL(18,2) NOT NULL,
    CONSTRAINT PK_detalle_cotizaciones PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_detcotiz_cotizaciones FOREIGN KEY (id_cotizacion) REFERENCES cotizaciones(id),
    CONSTRAINT FK_detcotiz_productos FOREIGN KEY (id_producto) REFERENCES productos(id),
    CONSTRAINT CK_detcotiz_cantidad CHECK (cantidad > 0),
    CONSTRAINT CK_detcotiz_precio CHECK (precio_unitario >= 0),
    CONSTRAINT CK_detcotiz_subtotal CHECK (subtotal >= 0)
);
GO

-- ============================================================
-- 16. VENTAS
-- ============================================================
CREATE TABLE ventas (
    id               INT IDENTITY(1,1) NOT NULL,
    id_cotizacion    INT            NULL,
    id_cliente       INT            NOT NULL,
    id_vendedor      INT            NOT NULL,
    fecha            DATETIME       NOT NULL CONSTRAINT DF_ventas_fecha DEFAULT GETDATE(),
    subtotal         DECIMAL(18,2)  NOT NULL,
    impuesto         DECIMAL(18,2)  NOT NULL,
    total            DECIMAL(18,2)  NOT NULL,
    estado           NVARCHAR(20)   NOT NULL CONSTRAINT DF_ventas_estado DEFAULT N'Pendiente',
    forma_pago       NVARCHAR(30)   NOT NULL,
    pago_recibido    DECIMAL(18,2)  NOT NULL CONSTRAINT DF_ventas_pago_recibido DEFAULT 0,
    requiere_envio   BIT            NOT NULL CONSTRAINT DF_ventas_requiere_envio DEFAULT 0,
    fecha_facturacion DATETIME      NULL,
    CONSTRAINT PK_ventas PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_ventas_cotizaciones FOREIGN KEY (id_cotizacion) REFERENCES cotizaciones(id),
    CONSTRAINT FK_ventas_clientes FOREIGN KEY (id_cliente) REFERENCES clientes(id),
    CONSTRAINT FK_ventas_vendedores FOREIGN KEY (id_vendedor) REFERENCES vendedores(id),
    CONSTRAINT CK_ventas_estado CHECK (estado IN (N'Pendiente', N'Confirmada', N'Enviada', N'Entregada', N'Cancelada', N'Devuelta')),
    CONSTRAINT CK_ventas_forma_pago CHECK (forma_pago IN (N'Efectivo', N'Tarjeta credito', N'Tarjeta debito', N'Transferencia', N'PSE', N'Nequi', N'Daviplata', N'QR')),
    CONSTRAINT CK_ventas_subtotal CHECK (subtotal >= 0),
    CONSTRAINT CK_ventas_impuesto CHECK (impuesto >= 0),
    CONSTRAINT CK_ventas_total CHECK (total >= 0),
    CONSTRAINT CK_ventas_pago_recibido CHECK (pago_recibido >= 0)
);
GO

-- ============================================================
-- 17. DETALLE_VENTAS
-- ============================================================
CREATE TABLE detalle_ventas (
    id                  INT IDENTITY(1,1) NOT NULL,
    id_venta            INT           NOT NULL,
    id_producto         INT           NOT NULL,
    cantidad            INT           NOT NULL,
    precio_unitario     DECIMAL(18,2) NOT NULL,
    comision_porcentaje DECIMAL(5,2)  NOT NULL CONSTRAINT DF_detventa_comision DEFAULT 0,
    subtotal            DECIMAL(18,2) NOT NULL,
    CONSTRAINT PK_detalle_ventas PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_detventas_ventas FOREIGN KEY (id_venta) REFERENCES ventas(id),
    CONSTRAINT FK_detventas_productos FOREIGN KEY (id_producto) REFERENCES productos(id),
    CONSTRAINT CK_detventas_cantidad CHECK (cantidad > 0),
    CONSTRAINT CK_detventas_precio CHECK (precio_unitario >= 0),
    CONSTRAINT CK_detventas_comision CHECK (comision_porcentaje >= 0 AND comision_porcentaje <= 100),
    CONSTRAINT CK_detventas_subtotal CHECK (subtotal >= 0)
);
GO

-- ============================================================
-- 18. HISTORICO_ESTADOS_VENTA
-- ============================================================
CREATE TABLE historico_estados_venta (
    id             INT IDENTITY(1,1) NOT NULL,
    id_venta       INT          NOT NULL,
    estado_anterior NVARCHAR(20) NOT NULL,
    estado_nuevo   NVARCHAR(20) NOT NULL,
    fecha_cambio   DATETIME     NOT NULL CONSTRAINT DF_histestados_fecha DEFAULT GETDATE(),
    id_usuario     INT          NOT NULL,
    CONSTRAINT PK_historico_estados_venta PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_histestados_ventas FOREIGN KEY (id_venta) REFERENCES ventas(id),
    CONSTRAINT FK_histestados_usuarios FOREIGN KEY (id_usuario) REFERENCES usuarios(id)
);
GO

-- ============================================================
-- 19. METODOS_PAGO
-- ============================================================
CREATE TABLE metodos_pago (
    id          INT IDENTITY(1,1) NOT NULL,
    nombre      NVARCHAR(50)  NOT NULL,
    descripcion NVARCHAR(200) NULL,
    activo      BIT           NOT NULL CONSTRAINT DF_metodospago_activo DEFAULT 1,
    CONSTRAINT PK_metodos_pago PRIMARY KEY CLUSTERED (id),
    CONSTRAINT UQ_metodos_pago_nombre UNIQUE (nombre)
);
GO

-- ============================================================
-- 20. PAGOS_VENTAS
-- ============================================================
CREATE TABLE pagos_ventas (
    id               INT IDENTITY(1,1) NOT NULL,
    id_venta         INT            NOT NULL,
    id_metodo_pago   INT            NOT NULL,
    monto            DECIMAL(18,2)  NOT NULL,
    referencia       NVARCHAR(100)  NULL,
    fecha            DATETIME       NOT NULL CONSTRAINT DF_pagosventas_fecha DEFAULT GETDATE(),
    estado           NVARCHAR(20)   NOT NULL CONSTRAINT DF_pagosventas_estado DEFAULT N'Pendiente',
    CONSTRAINT PK_pagos_ventas PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_pagosventas_ventas FOREIGN KEY (id_venta) REFERENCES ventas(id),
    CONSTRAINT FK_pagosventas_metodospago FOREIGN KEY (id_metodo_pago) REFERENCES metodos_pago(id),
    CONSTRAINT CK_pagosventas_monto CHECK (monto > 0),
    CONSTRAINT CK_pagosventas_estado CHECK (estado IN (N'Pendiente', N'Completado', N'Fallido', N'Revertido'))
);
GO

-- ============================================================
-- 21. COMISIONES
-- ============================================================
CREATE TABLE comisiones (
    id                  INT IDENTITY(1,1) NOT NULL,
    id_venta            INT           NOT NULL,
    id_vendedor         INT           NOT NULL,
    id_detalle_venta    INT           NOT NULL,
    monto_base          DECIMAL(18,2) NOT NULL,
    porcentaje          DECIMAL(5,2)  NOT NULL,
    monto_comision      DECIMAL(18,2) NOT NULL,
    bono                DECIMAL(18,2) NOT NULL CONSTRAINT DF_comisiones_bono DEFAULT 0,
    periodo             NVARCHAR(20)  NOT NULL,
    estado              NVARCHAR(20)  NOT NULL CONSTRAINT DF_comisiones_estado DEFAULT N'Pendiente',
    CONSTRAINT PK_comisiones PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_comisiones_ventas FOREIGN KEY (id_venta) REFERENCES ventas(id),
    CONSTRAINT FK_comisiones_vendedores FOREIGN KEY (id_vendedor) REFERENCES vendedores(id),
    CONSTRAINT FK_comisiones_detventas FOREIGN KEY (id_detalle_venta) REFERENCES detalle_ventas(id),
    CONSTRAINT CK_comisiones_monto_base CHECK (monto_base >= 0),
    CONSTRAINT CK_comisiones_porcentaje CHECK (porcentaje >= 0 AND porcentaje <= 100),
    CONSTRAINT CK_comisiones_monto CHECK (monto_comision >= 0),
    CONSTRAINT CK_comisiones_bono CHECK (bono >= 0),
    CONSTRAINT CK_comisiones_estado CHECK (estado IN (N'Pendiente', N'Aprobada', N'Pagada', N'Rechazada'))
);
GO

-- ============================================================
-- 22. METAS_VENTAS
-- ============================================================
CREATE TABLE metas_ventas (
    id           INT IDENTITY(1,1) NOT NULL,
    id_vendedor  INT            NOT NULL,
    periodo      NVARCHAR(20)  NOT NULL,
    monto_meta   DECIMAL(18,2) NOT NULL,
    monto_logrado DECIMAL(18,2) NOT NULL CONSTRAINT DF_metas_logrado DEFAULT 0,
    cumplio      BIT           NOT NULL CONSTRAINT DF_metas_cumplio DEFAULT 0,
    CONSTRAINT PK_metas_ventas PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_metas_vendedores FOREIGN KEY (id_vendedor) REFERENCES vendedores(id),
    CONSTRAINT UQ_metas_vendedor_periodo UNIQUE (id_vendedor, periodo),
    CONSTRAINT CK_metas_monto_meta CHECK (monto_meta > 0),
    CONSTRAINT CK_metas_monto_logrado CHECK (monto_logrado >= 0)
);
GO

-- ============================================================
-- 23. BONOS
-- ============================================================
CREATE TABLE bonos (
    id             INT IDENTITY(1,1) NOT NULL,
    id_comision    INT            NOT NULL,
    tipo           NVARCHAR(50)   NOT NULL,
    monto          DECIMAL(18,2)  NOT NULL,
    descripcion    NVARCHAR(200)  NULL,
    CONSTRAINT PK_bonos PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_bonos_comisiones FOREIGN KEY (id_comision) REFERENCES comisiones(id),
    CONSTRAINT CK_bonos_tipo CHECK (tipo IN (N'Meta cumplida', N'Venta especial', N'Bono temporada', N'Descuento volumen', N'Otro')),
    CONSTRAINT CK_bonos_monto CHECK (monto >= 0)
);
GO

-- ============================================================
-- 24. PAGOS_COMISIONES
-- ============================================================
CREATE TABLE pagos_comisiones (
    id             INT IDENTITY(1,1) NOT NULL,
    id_comision    INT            NOT NULL,
    id_vendedor    INT            NOT NULL,
    id_metodo_pago INT            NOT NULL,
    monto          DECIMAL(18,2)  NOT NULL,
    fecha_pago     DATETIME       NOT NULL CONSTRAINT DF_pagoscomisiones_fecha DEFAULT GETDATE(),
    periodo        NVARCHAR(20)   NOT NULL,
    estado         NVARCHAR(20)   NOT NULL CONSTRAINT DF_pagoscomisiones_estado DEFAULT N'Pendiente',
    referencia     NVARCHAR(100)  NULL,
    CONSTRAINT PK_pagos_comisiones PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_pagoscomisiones_comisiones FOREIGN KEY (id_comision) REFERENCES comisiones(id),
    CONSTRAINT FK_pagoscomisiones_vendedores FOREIGN KEY (id_vendedor) REFERENCES vendedores(id),
    CONSTRAINT FK_pagoscomisiones_metodospago FOREIGN KEY (id_metodo_pago) REFERENCES metodos_pago(id),
    CONSTRAINT CK_pagoscomisiones_monto CHECK (monto > 0),
    CONSTRAINT CK_pagoscomisiones_estado CHECK (estado IN (N'Pendiente', N'Procesado', N'Completado', N'Fallido'))
);
GO

-- ============================================================
-- 25. CONCILIACIONES_PAGO
-- ============================================================
CREATE TABLE conciliaciones_pago (
    id                 INT IDENTITY(1,1) NOT NULL,
    id_pago_comision   INT            NOT NULL,
    monto_conciliado   DECIMAL(18,2)  NOT NULL,
    fecha_conciliacion DATETIME       NOT NULL CONSTRAINT DF_conciliaciones_fecha DEFAULT GETDATE(),
    estado             NVARCHAR(20)   NOT NULL CONSTRAINT DF_conciliaciones_estado DEFAULT N'Pendiente',
    observaciones      NVARCHAR(500)  NULL,
    CONSTRAINT PK_conciliaciones_pago PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_conciliaciones_pagoscomisiones FOREIGN KEY (id_pago_comision) REFERENCES pagos_comisiones(id),
    CONSTRAINT CK_conciliaciones_monto CHECK (monto_conciliado >= 0),
    CONSTRAINT CK_conciliaciones_estado CHECK (estado IN (N'Pendiente', N'Conciliado', N'Discrepancia', N'Revisado'))
);
GO

-- ============================================================
-- 26. REPORTES_PROGRAMADOS
-- ============================================================
CREATE TABLE reportes_programados (
    id                 INT IDENTITY(1,1) NOT NULL,
    nombre             NVARCHAR(100)  NOT NULL,
    descripcion        NVARCHAR(500)  NULL,
    tipo_reporte       NVARCHAR(50)   NOT NULL,
    formato            NVARCHAR(20)   NOT NULL CONSTRAINT DF_reportes_formato DEFAULT N'PDF',
    query              NVARCHAR(MAX)  NOT NULL,
    destinatarios_email NVARCHAR(500) NULL,
    programacion       NVARCHAR(50)   NOT NULL,
    activo             BIT            NOT NULL CONSTRAINT DF_reportes_activo DEFAULT 1,
    ultima_ejecucion   DATETIME       NULL,
    proxima_ejecucion  DATETIME       NULL,
    CONSTRAINT PK_reportes_programados PRIMARY KEY CLUSTERED (id),
    CONSTRAINT CK_reportes_tipo CHECK (tipo_reporte IN (N'Ventas', N'Comisiones', N'Inventario', N'Clientes', N'Vendedores', N'General')),
    CONSTRAINT CK_reportes_formato CHECK (formato IN (N'PDF', N'Excel', N'CSV', N'HTML'))
);
GO

-- ============================================================
-- 27. HISTORIAL_EJECUCIONES
-- ============================================================
CREATE TABLE historial_ejecuciones (
    id               INT IDENTITY(1,1) NOT NULL,
    id_reporte       INT          NOT NULL,
    fecha_ejecucion  DATETIME     NOT NULL CONSTRAINT DF_historial_ejec_fecha DEFAULT GETDATE(),
    estado           NVARCHAR(20) NOT NULL,
    archivo_generado NVARCHAR(200) NULL,
    errores          NVARCHAR(MAX) NULL,
    CONSTRAINT PK_historial_ejecuciones PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_historial_reportes FOREIGN KEY (id_reporte) REFERENCES reportes_programados(id),
    CONSTRAINT CK_historial_estado CHECK (estado IN (N'Exitoso', N'Fallido', N'En proceso'))
);
GO

-- ============================================================
-- 28. CONFIGURACION_SISTEMA
-- ============================================================
CREATE TABLE configuracion_sistema (
    id          INT IDENTITY(1,1) NOT NULL,
    clave       NVARCHAR(100) NOT NULL,
    valor       NVARCHAR(500) NOT NULL,
    descripcion NVARCHAR(300) NULL,
    CONSTRAINT PK_configuracion_sistema PRIMARY KEY CLUSTERED (id),
    CONSTRAINT UQ_configuracion_clave UNIQUE (clave)
);
GO

-- ============================================================
-- 29. AUDITORIA_ACCIONES
-- ============================================================
CREATE TABLE auditoria_acciones (
    id               INT IDENTITY(1,1) NOT NULL,
    id_usuario       INT            NULL,
    tabla_afectada   NVARCHAR(100)  NOT NULL,
    id_registro      INT            NULL,
    accion           NVARCHAR(20)   NOT NULL,
    valores_anteriores NVARCHAR(MAX) NULL,
    valores_nuevos   NVARCHAR(MAX)  NULL,
    fecha            DATETIME       NOT NULL CONSTRAINT DF_auditoria_fecha DEFAULT GETDATE(),
    ip_address       NVARCHAR(45)   NULL,
    CONSTRAINT PK_auditoria_acciones PRIMARY KEY CLUSTERED (id),
    CONSTRAINT FK_auditoria_usuarios FOREIGN KEY (id_usuario) REFERENCES usuarios(id),
    CONSTRAINT CK_auditoria_accion CHECK (accion IN (N'INSERT', N'UPDATE', N'DELETE'))
);
GO

-- ============================================================
-- INDEXES
-- ============================================================

-- usuarios
CREATE INDEX IX_usuarios_id_rol ON usuarios(id_rol);
CREATE INDEX IX_usuarios_activo ON usuarios(activo);
GO

-- roles_permisos
CREATE INDEX IX_rolespermisos_id_permiso ON roles_permisos(id_permiso);
GO

-- vendedores
CREATE INDEX IX_vendedores_id_usuario ON vendedores(id_usuario);
CREATE INDEX IX_vendedores_id_supervisor ON vendedores(id_supervisor);
CREATE INDEX IX_vendedores_activo ON vendedores(activo);
CREATE INDEX IX_vendedores_identificacion ON vendedores(identificacion);
GO

-- supervisores
CREATE INDEX IX_supervisores_id_vendedor ON supervisores(id_vendedor);
GO

-- vendedores_zonas
CREATE INDEX IX_vz_id_zona ON vendedores_zonas(id_zona);
GO

-- contactos_emergencia
CREATE INDEX IX_contactos_id_vendedor ON contactos_emergencia(id_vendedor);
GO

-- datos_bancarios
CREATE INDEX IX_datosbancarios_id_vendedor ON datos_bancarios(id_vendedor);
CREATE INDEX IX_datosbancarios_activo ON datos_bancarios(activo);
GO

-- clientes
CREATE INDEX IX_clientes_identificacion ON clientes(identificacion);
CREATE INDEX IX_clientes_activo ON clientes(activo);
GO

-- productos
CREATE INDEX IX_productos_id_categoria ON productos(id_categoria);
CREATE INDEX IX_productos_activo ON productos(activo);
CREATE INDEX IX_productos_nombre ON productos(nombre);
GO

-- cotizaciones
CREATE INDEX IX_cotizaciones_id_cliente ON cotizaciones(id_cliente);
CREATE INDEX IX_cotizaciones_id_vendedor ON cotizaciones(id_vendedor);
CREATE INDEX IX_cotizaciones_fecha ON cotizaciones(fecha);
CREATE INDEX IX_cotizaciones_estado ON cotizaciones(estado);
GO

-- detalle_cotizaciones
CREATE INDEX IX_detcotiz_id_cotizacion ON detalle_cotizaciones(id_cotizacion);
CREATE INDEX IX_detcotiz_id_producto ON detalle_cotizaciones(id_producto);
GO

-- ventas
CREATE INDEX IX_ventas_id_cotizacion ON ventas(id_cotizacion);
CREATE INDEX IX_ventas_id_cliente ON ventas(id_cliente);
CREATE INDEX IX_ventas_id_vendedor ON ventas(id_vendedor);
CREATE INDEX IX_ventas_fecha ON ventas(fecha);
CREATE INDEX IX_ventas_estado ON ventas(estado);
CREATE INDEX IX_ventas_forma_pago ON ventas(forma_pago);
GO

-- detalle_ventas
CREATE INDEX IX_detventas_id_venta ON detalle_ventas(id_venta);
CREATE INDEX IX_detventas_id_producto ON detalle_ventas(id_producto);
GO

-- historico_estados_venta
CREATE INDEX IX_histestados_id_venta ON historico_estados_venta(id_venta);
CREATE INDEX IX_histestados_id_usuario ON historico_estados_venta(id_usuario);
CREATE INDEX IX_histestados_fecha ON historico_estados_venta(fecha_cambio);
GO

-- pagos_ventas
CREATE INDEX IX_pagosventas_id_venta ON pagos_ventas(id_venta);
CREATE INDEX IX_pagosventas_id_metodo_pago ON pagos_ventas(id_metodo_pago);
CREATE INDEX IX_pagosventas_estado ON pagos_ventas(estado);
CREATE INDEX IX_pagosventas_fecha ON pagos_ventas(fecha);
GO

-- comisiones
CREATE INDEX IX_comisiones_id_venta ON comisiones(id_venta);
CREATE INDEX IX_comisiones_id_vendedor ON comisiones(id_vendedor);
CREATE INDEX IX_comisiones_id_detalle_venta ON comisiones(id_detalle_venta);
CREATE INDEX IX_comisiones_periodo ON comisiones(periodo);
CREATE INDEX IX_comisiones_estado ON comisiones(estado);
GO

-- metas_ventas
CREATE INDEX IX_metas_id_vendedor ON metas_ventas(id_vendedor);
CREATE INDEX IX_metas_periodo ON metas_ventas(periodo);
GO

-- bonos
CREATE INDEX IX_bonos_id_comision ON bonos(id_comision);
GO

-- pagos_comisiones
CREATE INDEX IX_pagoscomisiones_id_comision ON pagos_comisiones(id_comision);
CREATE INDEX IX_pagoscomisiones_id_vendedor ON pagos_comisiones(id_vendedor);
CREATE INDEX IX_pagoscomisiones_id_metodo_pago ON pagos_comisiones(id_metodo_pago);
CREATE INDEX IX_pagoscomisiones_estado ON pagos_comisiones(estado);
CREATE INDEX IX_pagoscomisiones_periodo ON pagos_comisiones(periodo);
CREATE INDEX IX_pagoscomisiones_fecha ON pagos_comisiones(fecha_pago);
GO

-- conciliaciones_pago
CREATE INDEX IX_conciliaciones_id_pago_comision ON conciliaciones_pago(id_pago_comision);
CREATE INDEX IX_conciliaciones_estado ON conciliaciones_pago(estado);
GO

-- historial_ejecuciones
CREATE INDEX IX_historial_id_reporte ON historial_ejecuciones(id_reporte);
CREATE INDEX IX_historial_fecha ON historial_ejecuciones(fecha_ejecucion);
CREATE INDEX IX_historial_estado ON historial_ejecuciones(estado);
GO

-- auditoria_acciones
CREATE INDEX IX_auditoria_id_usuario ON auditoria_acciones(id_usuario);
CREATE INDEX IX_auditoria_tabla ON auditoria_acciones(tabla_afectada);
CREATE INDEX IX_auditoria_accion ON auditoria_acciones(accion);
CREATE INDEX IX_auditoria_fecha ON auditoria_acciones(fecha);
CREATE INDEX IX_auditoria_id_registro ON auditoria_acciones(id_registro);
GO
