import React from "react";
import {
  List,
  Datagrid,
  TextField,
  ReferenceField,
  EditButton,
} from "react-admin";

export const UserList = props => (
  <List {...props}>
    <Datagrid>
      <TextField source="id" />
      <ReferenceField source="id" reference="users">
        <TextField source="username" />
      </ReferenceField>
      <TextField source="website" />
      <EditButton />
    </Datagrid>
  </List>
);
