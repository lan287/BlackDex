/*
 * This file is auto-generated.  DO NOT MODIFY.
 */
package top.niunaijun.blackbox.core.system.dump;
public interface IBDumpManagerService extends android.os.IInterface
{
  /** Default implementation for IBDumpManagerService. */
  public static class Default implements top.niunaijun.blackbox.core.system.dump.IBDumpManagerService
  {
    @Override public void registerMonitor(android.os.IBinder monitor) throws android.os.RemoteException
    {
    }
    @Override public void unregisterMonitor(android.os.IBinder monitor) throws android.os.RemoteException
    {
    }
    @Override public void noticeMonitor(top.niunaijun.blackbox.entity.dump.DumpResult result) throws android.os.RemoteException
    {
    }
    @Override
    public android.os.IBinder asBinder() {
      return null;
    }
  }
  /** Local-side IPC implementation stub class. */
  public static abstract class Stub extends android.os.Binder implements top.niunaijun.blackbox.core.system.dump.IBDumpManagerService
  {
    /** Construct the stub at attach it to the interface. */
    public Stub()
    {
      this.attachInterface(this, DESCRIPTOR);
    }
    /**
     * Cast an IBinder object into an top.niunaijun.blackbox.core.system.dump.IBDumpManagerService interface,
     * generating a proxy if needed.
     */
    public static top.niunaijun.blackbox.core.system.dump.IBDumpManagerService asInterface(android.os.IBinder obj)
    {
      if ((obj==null)) {
        return null;
      }
      android.os.IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
      if (((iin!=null)&&(iin instanceof top.niunaijun.blackbox.core.system.dump.IBDumpManagerService))) {
        return ((top.niunaijun.blackbox.core.system.dump.IBDumpManagerService)iin);
      }
      return new top.niunaijun.blackbox.core.system.dump.IBDumpManagerService.Stub.Proxy(obj);
    }
    @Override public android.os.IBinder asBinder()
    {
      return this;
    }
    @Override public boolean onTransact(int code, android.os.Parcel data, android.os.Parcel reply, int flags) throws android.os.RemoteException
    {
      java.lang.String descriptor = DESCRIPTOR;
      if (code >= android.os.IBinder.FIRST_CALL_TRANSACTION && code <= android.os.IBinder.LAST_CALL_TRANSACTION) {
        data.enforceInterface(descriptor);
      }
      switch (code)
      {
        case INTERFACE_TRANSACTION:
        {
          reply.writeString(descriptor);
          return true;
        }
      }
      switch (code)
      {
        case TRANSACTION_registerMonitor:
        {
          android.os.IBinder _arg0;
          _arg0 = data.readStrongBinder();
          this.registerMonitor(_arg0);
          reply.writeNoException();
          break;
        }
        case TRANSACTION_unregisterMonitor:
        {
          android.os.IBinder _arg0;
          _arg0 = data.readStrongBinder();
          this.unregisterMonitor(_arg0);
          reply.writeNoException();
          break;
        }
        case TRANSACTION_noticeMonitor:
        {
          top.niunaijun.blackbox.entity.dump.DumpResult _arg0;
          _arg0 = _Parcel.readTypedObject(data, top.niunaijun.blackbox.entity.dump.DumpResult.CREATOR);
          this.noticeMonitor(_arg0);
          reply.writeNoException();
          break;
        }
        default:
        {
          return super.onTransact(code, data, reply, flags);
        }
      }
      return true;
    }
    private static class Proxy implements top.niunaijun.blackbox.core.system.dump.IBDumpManagerService
    {
      private android.os.IBinder mRemote;
      Proxy(android.os.IBinder remote)
      {
        mRemote = remote;
      }
      @Override public android.os.IBinder asBinder()
      {
        return mRemote;
      }
      public java.lang.String getInterfaceDescriptor()
      {
        return DESCRIPTOR;
      }
      @Override public void registerMonitor(android.os.IBinder monitor) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeStrongBinder(monitor);
          boolean _status = mRemote.transact(Stub.TRANSACTION_registerMonitor, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      @Override public void unregisterMonitor(android.os.IBinder monitor) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeStrongBinder(monitor);
          boolean _status = mRemote.transact(Stub.TRANSACTION_unregisterMonitor, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      @Override public void noticeMonitor(top.niunaijun.blackbox.entity.dump.DumpResult result) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _Parcel.writeTypedObject(_data, result, 0);
          boolean _status = mRemote.transact(Stub.TRANSACTION_noticeMonitor, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
    }
    static final int TRANSACTION_registerMonitor = (android.os.IBinder.FIRST_CALL_TRANSACTION + 0);
    static final int TRANSACTION_unregisterMonitor = (android.os.IBinder.FIRST_CALL_TRANSACTION + 1);
    static final int TRANSACTION_noticeMonitor = (android.os.IBinder.FIRST_CALL_TRANSACTION + 2);
  }
  public static final java.lang.String DESCRIPTOR = "top.niunaijun.blackbox.core.system.dump.IBDumpManagerService";
  public void registerMonitor(android.os.IBinder monitor) throws android.os.RemoteException;
  public void unregisterMonitor(android.os.IBinder monitor) throws android.os.RemoteException;
  public void noticeMonitor(top.niunaijun.blackbox.entity.dump.DumpResult result) throws android.os.RemoteException;
  /** @hide */
  static class _Parcel {
    static private <T> T readTypedObject(
        android.os.Parcel parcel,
        android.os.Parcelable.Creator<T> c) {
      if (parcel.readInt() != 0) {
          return c.createFromParcel(parcel);
      } else {
          return null;
      }
    }
    static private <T extends android.os.Parcelable> void writeTypedObject(
        android.os.Parcel parcel, T value, int parcelableFlags) {
      if (value != null) {
        parcel.writeInt(1);
        value.writeToParcel(parcel, parcelableFlags);
      } else {
        parcel.writeInt(0);
      }
    }
  }
}
