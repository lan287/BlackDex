/*
 * This file is auto-generated.  DO NOT MODIFY.
 */
package top.niunaijun.blackbox.core.system.os;
// Declare any non-default types here with import statements
public interface IBStorageManagerService extends android.os.IInterface
{
  /** Default implementation for IBStorageManagerService. */
  public static class Default implements top.niunaijun.blackbox.core.system.os.IBStorageManagerService
  {
    @Override public android.os.storage.StorageVolume[] getVolumeList(int uid, java.lang.String packageName, int flags, int userId) throws android.os.RemoteException
    {
      return null;
    }
    @Override
    public android.os.IBinder asBinder() {
      return null;
    }
  }
  /** Local-side IPC implementation stub class. */
  public static abstract class Stub extends android.os.Binder implements top.niunaijun.blackbox.core.system.os.IBStorageManagerService
  {
    /** Construct the stub at attach it to the interface. */
    public Stub()
    {
      this.attachInterface(this, DESCRIPTOR);
    }
    /**
     * Cast an IBinder object into an top.niunaijun.blackbox.core.system.os.IBStorageManagerService interface,
     * generating a proxy if needed.
     */
    public static top.niunaijun.blackbox.core.system.os.IBStorageManagerService asInterface(android.os.IBinder obj)
    {
      if ((obj==null)) {
        return null;
      }
      android.os.IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
      if (((iin!=null)&&(iin instanceof top.niunaijun.blackbox.core.system.os.IBStorageManagerService))) {
        return ((top.niunaijun.blackbox.core.system.os.IBStorageManagerService)iin);
      }
      return new top.niunaijun.blackbox.core.system.os.IBStorageManagerService.Stub.Proxy(obj);
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
        case TRANSACTION_getVolumeList:
        {
          int _arg0;
          _arg0 = data.readInt();
          java.lang.String _arg1;
          _arg1 = data.readString();
          int _arg2;
          _arg2 = data.readInt();
          int _arg3;
          _arg3 = data.readInt();
          android.os.storage.StorageVolume[] _result = this.getVolumeList(_arg0, _arg1, _arg2, _arg3);
          reply.writeNoException();
          reply.writeTypedArray(_result, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
          break;
        }
        default:
        {
          return super.onTransact(code, data, reply, flags);
        }
      }
      return true;
    }
    private static class Proxy implements top.niunaijun.blackbox.core.system.os.IBStorageManagerService
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
      @Override public android.os.storage.StorageVolume[] getVolumeList(int uid, java.lang.String packageName, int flags, int userId) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        android.os.storage.StorageVolume[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeInt(uid);
          _data.writeString(packageName);
          _data.writeInt(flags);
          _data.writeInt(userId);
          boolean _status = mRemote.transact(Stub.TRANSACTION_getVolumeList, _data, _reply, 0);
          _reply.readException();
          _result = _reply.createTypedArray(android.os.storage.StorageVolume.CREATOR);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
    }
    static final int TRANSACTION_getVolumeList = (android.os.IBinder.FIRST_CALL_TRANSACTION + 0);
  }
  public static final java.lang.String DESCRIPTOR = "top.niunaijun.blackbox.core.system.os.IBStorageManagerService";
  public android.os.storage.StorageVolume[] getVolumeList(int uid, java.lang.String packageName, int flags, int userId) throws android.os.RemoteException;
}
