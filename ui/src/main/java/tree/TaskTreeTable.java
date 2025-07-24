package tree;

import config.TaskConfig;
import data.Task;
import data.TaskState;
import dialogs.AddModifyTask;
import io.github.andrewauclair.moderndocking.app.Docking;
import net.byteseek.swing.treetable.TreeTableHeaderRenderer;
import net.byteseek.swing.treetable.TreeTableModel;
import net.byteseek.swing.treetable.TreeUtils;
import packets.PacketType;
import packets.RequestID;
import packets.TaskStateChange;
import packets.UpdateTask;
import panels.TasksList;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreeNode;
import java.awt.datatransfer.StringSelection;
import java.awt.datatransfer.Transferable;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;
import java.util.List;

/**
 * A tree table that can hold tasks. Reusable for the tasks list, system tray search, reports, and task selector
 */
public class TaskTreeTable extends JTable {
    class TaskTransferHandler extends TransferHandler {
        private List<Integer> rows = new ArrayList<>();

        @Override
        public int getSourceActions(JComponent c) {
            return MOVE;
        }

        @Override
        protected Transferable createTransferable(JComponent c) {
            int[] selectedRows = getSelectedRows();

            rows = new ArrayList<>();

            for (int i = selectedRows.length - 1; i >= 0; i--) {
                DefaultMutableTreeNode nodeAtTableRow = (DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(selectedRows[i]);
                Task task = (Task) nodeAtTableRow.getUserObject();

                if (!task.serverControlled && !task.locked) {
                    rows.add(selectedRows[i]);
                }
            }

            if (rows.isEmpty()) {
                return null;
            }
            return new StringSelection("");
        }

        @Override
        public boolean canImport(TransferSupport support) {
            return true;
        }

        @Override
        public boolean importData(TransferSupport support) {
            // send to server
            for (int row : rows) {
                DefaultMutableTreeNode nodeAtTableRow = (DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(row);
                Task task = (Task) nodeAtTableRow.getUserObject();

                JTable.DropLocation dl = (JTable.DropLocation) support.getDropLocation();

                DefaultMutableTreeNode dropNode = (DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(dl.getRow());

                // dropping after the last node on the tree
                if (dropNode == null) {
                    System.out.println("insert as last task in root list");


                }
                else {
                    Task dropTask = (Task) dropNode.getUserObject();
                    Task parentTask;

                    // the drop row is always the row below where the insert line is
                    if (dl.isInsertRow()) {
                        parentTask = mainFrame.getTaskModel().getTask(dropTask.parentID);
                    }
                    else {
                        parentTask = dropTask;
                    }
                    int index = dl.isInsertRow() ? dropTask.indexInParent : 0;

                    if (parentTask == null) {
                        System.out.println("insert with root as parent");

                        UpdateTask update = new UpdateTask(RequestID.nextRequestID(), task.id, 0, task.name);
                        update.indexInParent = index;
                        mainFrame.getConnection().sendPacket(update);
                    }
                    else {
                        System.out.println("Reparent '" + task.name + "' to '" + parentTask.name);

                        UpdateTask update = new UpdateTask(RequestID.nextRequestID(), task.id, parentTask.id, task.name);
                        update.indexInParent = index;
                        mainFrame.getConnection().sendPacket(update);
                    }
                    // move task: request id, task id, new parent id

                }
            }
            return true;
        }
    }

    private MainFrame mainFrame;
    private DefaultMutableTreeNode rootNode;
    private TreeTableModel treeTableModel;
    private DefaultTreeModel treeModel;

    public TaskTreeTable(Task root) {
        rootNode = TreeUtils.buildTree(root, Task::getChildren, parent -> false);

        setSelectionMode(ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);
        setDragEnabled(true);
        setDropMode(DropMode.ON_OR_INSERT_ROWS);
        setTransferHandler(new TaskTransferHandler());

        treeTableModel.setNodeFilter(treeNode -> {
            // never filter the root. we keep it hidden
            if (treeNode == rootNode) {
                return false;
            }
            Task obj = TreeUtils.getUserObject(treeNode);
            return obj.state == TaskState.FINISHED;
        });

        JMenuItem add = new JMenuItem("Add Task...");
        JMenuItem addSubTask = new JMenuItem("Add Sub-Task...");
        JMenuItem start = new JMenuItem("Start");
        JMenuItem startStopActive = new JMenuItem("Start (Stop Active)");
        JMenuItem startFinishActive = new JMenuItem("Start (Finish Active)");
        JMenuItem stop = new JMenuItem("Stop");
        JMenuItem finish = new JMenuItem("Finish");
        JMenuItem openInNewWindow = new JMenuItem("Open in New List");
        JMenuItem config = new JMenuItem("Configure...");

        config.addActionListener(e -> {
            int selectedRow = getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            Task task = (Task) ((DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(selectedRow)).getUserObject();

            TaskConfig dialog = new TaskConfig(mainFrame, mainFrame, task);
            dialog.setVisible(true);
        });

        start.addActionListener(e -> changeTaskState(PacketType.START_TASK));
        startStopActive.addActionListener(e -> changeTaskState(PacketType.START_TASK));
        startFinishActive.addActionListener(e -> {
            finishActiveTask();
            changeTaskState(PacketType.START_TASK);
        });
        stop.addActionListener(e -> changeTaskState(PacketType.STOP_TASK));
        finish.addActionListener(e -> changeTaskState(PacketType.FINISH_TASK));

        add.addActionListener(e -> new AddModifyTask(mainFrame, mainFrame, 0, false).setVisible(true));

        addSubTask.addActionListener(e -> {
            int selectedRow = getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            Task task = (Task) ((DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(selectedRow)).getUserObject();

            new AddModifyTask(mainFrame, mainFrame, task.id, false).setVisible(true);
        });

        openInNewWindow.addActionListener(e -> {
            int selectedRow = getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            Task task = (Task) ((DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(selectedRow)).getUserObject();

            if (!Docking.isDockableRegistered("tasks-list-" + task.id)) {
                TasksList newList = new TasksList(mainFrame, task);
//                Docking.dock(newList, TasksList.this, DockingRegion.CENTER);
            }
            else {
                Docking.display("tasks-list-" + task.id);
            }
        });

        addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (SwingUtilities.isRightMouseButton(e)) {
                    int selectedRow = getSelectedRow();

                    JPopupMenu contextMenu = new JPopupMenu();

                    if (selectedRow == -1) {
                        contextMenu.add(add);
                        contextMenu.show(TaskTreeTable.this, e.getX(), e.getY());
                        return;
                    }

                    contextMenu.add(config);

                    if (mainFrame.getTaskModel().getActiveTaskID().isPresent() &&
                            !mainFrame.getTaskModel().taskHasNonFinishedChildren(mainFrame.getTaskModel().getActiveTaskID().get()) &&
                            !mainFrame.getTaskModel().getTask(mainFrame.getTaskModel().getActiveTaskID().get()).locked) {
                        contextMenu.add(startStopActive);
                        contextMenu.add(stop);
                        contextMenu.add(startFinishActive);
                        contextMenu.add(finish);
                    }
                    else {
                        contextMenu.add(start);
                        contextMenu.add(stop);
                        contextMenu.add(finish);
                    }


                    contextMenu.addSeparator();
                    contextMenu.add(addSubTask);

                    TreeNode node = treeTableModel.getNodeAtTableRow(selectedRow);
                    Task task = (Task) ((DefaultMutableTreeNode) node).getUserObject();

                    startStopActive.setEnabled(task.state == TaskState.PENDING);
                    startFinishActive.setEnabled(task.state == TaskState.PENDING);
                    stop.setEnabled(task.state == TaskState.ACTIVE);
                    finish.setEnabled(task.state != TaskState.FINISHED && !mainFrame.getTaskModel().taskHasNonFinishedChildren(task.id) && !task.locked && !task.serverControlled);

                    // task has subtasks, allow an option to open it in a new panel
                    if (node.getChildCount() != 0) {
                        contextMenu.addSeparator();
                        contextMenu.add(openInNewWindow);
                    }

                    contextMenu.show(TaskTreeTable.this, e.getX(), e.getY());
                }
                else if (e.getClickCount() == 2 && SwingUtilities.isLeftMouseButton(e)) {
                    config.doClick();
                }
            }
        });
    }

    private void finishActiveTask() {
        TaskStateChange change = new TaskStateChange();
        change.packetType = PacketType.FINISH_TASK;
        change.taskID = mainFrame.getTaskModel().getActiveTaskID().get();
        mainFrame.getConnection().sendPacket(change);
    }

    private void changeTaskState(PacketType type) {
        int selectedRow = getSelectedRow();

        if (selectedRow == -1) {
            return;
        }

        Task task = (Task) ((DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(selectedRow)).getUserObject();

        TaskStateChange change = new TaskStateChange();
        change.packetType = type;
        change.taskID = task.id;
        mainFrame.getConnection().sendPacket(change);
    }

    private boolean childrenHaveMatch(Task obj, String text) {
        for (Task child : obj.children) {
            if (childrenHaveMatch(child, text)) {
                return true;
            }
        }
        if (obj.name.toLowerCase().contains(text.toLowerCase())) {
            return true;
        }
        return false;
    }

    private DefaultTreeModel createTreeModel(TreeNode rootNode) {
        DefaultTreeModel model = new DefaultTreeModel(rootNode);
        model.addTreeModelListener(treeTableModel);
        return model;
    }

    private TreeTableModel createTreeTableModel(TreeNode rootNode) {
        TreeTableModel localTreeTableModel = new TaskTreeTableModel(rootNode, false);

        TreeTableHeaderRenderer renderer = new TreeTableHeaderRenderer();
//        renderer.setShowNumber(true); // true is default, this code is just for testing the false option.

        localTreeTableModel.bindTable(this, renderer);
        localTreeTableModel.addExpandCollapseListener(new TreeTableModel.ExpandCollapseListener() {
            @Override
            public boolean nodeExpanding(TreeNode node) {
                if (node.getChildCount() == 0) { // if a node is expanding but has no children, set it to allow no children.
                    ((DefaultMutableTreeNode) node).setAllowsChildren(false);
                }
                return true;
            }

            @Override
            public boolean nodeCollapsing(TreeNode node) {
                return true;
            }
        });
        return localTreeTableModel;
    }
}
